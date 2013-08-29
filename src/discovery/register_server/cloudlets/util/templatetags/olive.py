from django import template
from django.utils.html import conditional_escape
from django.utils.safestring import mark_safe
import markdown as md
from urllib import urlencode
from urlparse import urlunsplit

#from ..utils import FullTextHighlighter

register = template.Library()

@register.simple_tag
def active(a, b, cls='active'):
    if a == b:
        return cls
    else:
        return ''

@register.inclusion_tag('util/form_fields.html')
def form_fields(form):
    return {
        'form': form
    }

@register.inclusion_tag('util/messages.html', takes_context=True)
def messages(context):
    return {
        'messages': context['messages'],
    }

@register.filter
def markdown(value):
    return mark_safe(md.markdown(value, output_format='html5',
            safe_mode='escape'))

@register.simple_tag(takes_context=True)
def page(context, n):
    request = context['request']
    args = dict(request.GET)
    args['p'] = str(n)
    return urlunsplit(('', '', request.path, urlencode(args, True), ''))

@register.filter(needs_autoescape=True)
def highlight(text, query, autoescape=None):
    if autoescape:
        text = conditional_escape(text)
    return text
#return mark_safe(FullTextHighlighter(query).highlight(text))
