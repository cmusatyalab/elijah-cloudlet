from django.conf import settings
from django.core.urlresolvers import resolve
from django.http import Http404

def page(request):
    # Add URL name of current page as "page" context variable
    try:
        page = resolve(request.get_full_path()).url_name
    except Http404:
        page = None
    return {
        'page': page,
    }

def piwik(request):
    # Add context variables for Piwik settings
    return {
        'piwik_base_url': settings.PIWIK_BASE_URL,
        'piwik_site_id': settings.PIWIK_SITE_ID,
    }
