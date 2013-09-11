from django.conf.urls import patterns, include, url
from tastypie.api import Api
from cloudlets.cloudlet.api import CloudletResource
from django.conf import settings

v1_api = Api(api_name='v1')
v1_api.register(CloudletResource())

from django.conf.urls.defaults import *
from django.contrib import admin
admin.autodiscover()

urlpatterns = patterns('')

# file dropbox
if settings.DEBUG or settings.SERVE_STATIC:
    urlpatterns += patterns('',
	url(r'^static/(?P<path>.*)$', 'django.views.static.serve', {'document_root': settings.STATIC_ROOT}),
	url(r'^media/(?P<path>.*)$', 'django.views.static.serve', {'document_root': settings.MEDIA_ROOT}),
    )

urlpatterns += patterns('',

    url(r'', include('cloudlets.base.urls')),
    url(r'^cloudlets/', include('cloudlets.cloudlet.urls')),
    url(r'^vmimages/', include('cloudlets.vmimages.urls')),
    url(r'^accounts/', include('cloudlets.accounts.urls')),
    url(r'^overlays/', include('fileshack.urls', 'fileshack')),

    # Uncomment the admin/doc line below to enable admin documentation:
    # url(r'^admin/doc/', include('django.contrib.admindocs.urls')),
    # Uncomment the next line to enable the admin:
    url(r'^admin/', include(admin.site.urls)),
    (r'^api/', include(v1_api.urls)),
)
