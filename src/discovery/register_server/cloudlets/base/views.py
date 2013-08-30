from django.conf import settings
from django.contrib import messages
from django.contrib.auth.decorators import login_required
from django.http import Http404
from django.shortcuts import render
from django.template import TemplateDoesNotExist

from . import forms
from ..util.utils import send_template_mail


def index(request):
    return render(request, "index.html", {})


def generic_page(request, path, context=None):
    try:
        if path.endswith('.txt'):
            return render(request, 'base/pages/' + path,
                    content_type='text/plain')
        else:
            path = 'base/pages/' + path.strip('/')
            return render(request, [
                path + '/index.html',
                path + '.html',
            ], context)
    except TemplateDoesNotExist:
        raise Http404


def unauthenticated_index(request):
    # Special case for index
    return generic_page(request, '')

def contact(request):
    if request.user.is_authenticated():
        initial = {
            'name': request.user.get_full_name(),
            'email': request.user.email,
        }
    else:
        initial = {}
    if request.method == 'POST':
        form = forms.ContactForm(request.POST)
        if form.is_valid():
            from_email = '%s <%s>' % (
                form.cleaned_data['name'].translate({
                    ord('<'): None,
                    ord('>'): None,
                }),
                settings.DEFAULT_FROM_EMAIL,
            )
            headers = {
                'Reply-To': form.cleaned_data['email'],
            }
            send_template_mail(request, settings.CONTACT_RECIPIENTS,
                    form.cleaned_data,
                    'base/contact-email-subject.txt',
                    'base/contact-email-body.txt',
                    'base/contact-email-body.html',
                    from_email=from_email, headers=headers)
            messages.success(request, '**Your inquiry has been delivered!**\n\nSomeone from the Cloudlet team will get in touch with you shortly.')
            form = forms.ContactForm(initial=initial)
    else:
        form = forms.ContactForm(initial=initial)
    return render(request, 'base/contact.html', {
        'form': form,
    })

def discussion(request):
    if request.user.is_authenticated():
        initial = {
            'email': request.user.email,
        }
    else:
        initial = {}
    return render(request, 'base/discussion.html', {
        'form': forms.MailingListSubscribeForm(initial=initial),
    })
