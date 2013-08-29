from django.conf import settings
from django.contrib.sites.models import get_current_site
from django.core.mail import EmailMultiAlternatives
from django.core.paginator import Paginator, PageNotAnInteger, EmptyPage
from django.http import HttpResponse
from django.shortcuts import render
from django.template.loader import render_to_string
#from haystack.utils import Highlighter
import re

def send_template_mail(request, to, dictionary, subject, text, html=None,
        from_email=settings.DEFAULT_FROM_EMAIL, headers=None):
    if not headers:
        headers = {}
    ctx = {
        'protocol': request.is_secure() and 'https' or 'http',
        'site': get_current_site(request),
    }
    ctx.update(dictionary)
    r = lambda tmpl: render_to_string(tmpl, ctx)
    msg = EmailMultiAlternatives(
        subject=r(subject).strip(),
        body=r(text),
        from_email=from_email,
        to=to,
        headers=headers,
    )
    if html:
        msg.attach_alternative(r(html), 'text/html')
    msg.send()


def paginate(request, template, objects, **vars):
    p = Paginator(objects, 10)
    try:
        page = p.page(request.GET.get('p', 1))
    except PageNotAnInteger:
        page = p.page(1)
    except EmptyPage:
        page = p.page(p.num_pages)
    vars.update({
        'page': page,
        'neighbors': range(max(1, page.number - 5),
                min(p.num_pages, page.number + 5) + 1),
    })
    return render(request, template, vars)


class RangeNotSatisfiable(Exception):
    def __init__(self, length):
        self.length = length

    def get_response(self):
        response = HttpResponse('Range not satisfiable', status=416,
                content_type='text/plain')
        response['Content-Range'] = 'bytes */%d' % self.length
        return response


class ObjectRange(object):
    def __init__(self, request, length):
        '''Parse Range header.  Accepts length of entity, provides
        attributes start and count; these may be None if there was no valid
        range request.  Throws RangeNotSatisfiable if HTTP 1.1 requires us
        to return 416.'''
        # As specified by HTTP 1.1, section 14.35.1.
        # We don't support multiple ranges, since this would require us
        # to wrap them in multipart/byteranges
        self.start = None
        self.count = None
        self._length = length
        self._have_range = False
        range = request.META.get('HTTP_RANGE', None)
        if not range:
            return
        match = re.match('bytes=([0-9]+)?-([0-9]+)?$', range)
        if match is None:
            return
        start, end = match.group(1, 2)
        start = start and int(start)
        end = end and int(end)
        if (start is None and end is None) or \
                    (start is not None and end is not None and end < start):
            # Syntactically invalid, ignore
            return
        if start is None:
            # Suffix range; client wants the last N bytes
            if end == 0:
                raise RangeNotSatisfiable(length)
            start = max(length - end, 0)
            end = length - 1
        else:
            # Normal byterange
            if start >= length:
                raise RangeNotSatisfiable(length)
            if end is None:
                end = length - 1
            else:
                end = min(end, length - 1)
        self.start = start
        self.count = end - start + 1
        self._have_range = True

    def set_response_headers(self, response):
        response['Accept-Ranges'] = 'bytes'
        if self._have_range:
            response.status_code = 206
            response['Content-Length'] = str(self.count)
            response['Content-Range'] = 'bytes %d-%d/%d' % (self.start,
                    self.start + self.count - 1, self._length)
        else:
            response['Content-Length'] = str(self._length)


#class FullTextHighlighter(Highlighter):
#    '''A Highlighter that doesn't excerpt the input text.'''
#
#    def render_html(self, highlight_locations=None, start_offset=None,
#            end_offset=None):
#        return super(FullTextHighlighter, self).render_html(
#                highlight_locations, 0, len(self.text_block))
