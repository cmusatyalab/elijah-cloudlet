from django.conf.urls import patterns, include, url

from .views import *

urlpatterns = patterns('',
    url(r'^contact/$', contact, name='cloudlet-contact'),
    url(r'^discuss/$', discussion, name='cloudlet-discussion'),
    url(r'^about/$', generic_page, {'path': 'about'}, name='cloudlet-about'),

    #url(r'^demo/$', generic_page, {'path': 'demo'}, name='cloudlet-demo'),
    #url(r'^documents/$', generic_page, {'path': 'documents'}, name='cloudlet-documents'),
    #url(r'^origins/$', generic_page, {'path': 'origins'}, name='cloudlet-origins'),
    #url(r'^partners/$', generic_page, {'path': 'partners'}, name='cloudlet-partners'),
    #url(r'^press/$', generic_page, {'path': 'press'}, name='cloudlet-press'),
    #url(r'^software/$', generic_page, {'path': 'software'}, name='cloudlet-software'),
    #url(r'^team/$', generic_page, {'path': 'team'}, name='cloudlet-team'),
    #url(r'^uses/$', generic_page, {'path': 'uses'}, name='cloudlet-uses'),

    url(r'^robots.txt$', generic_page, {'path': 'robots.txt'}),
)
