from django.conf.urls import patterns, url, include
from django.contrib.auth.decorators import login_required
from .views import *

urlpatterns = patterns('',
    # ex: /cloudlets/
    url(r'^$', login_required(CloudletListView.as_view()), name='cloudlet-cloudlet-list'),
    #url(r'^$', all_images, name='cloudlet-cloudlet-list'),
    # Ex: /cloudlets/5/
    url(r'^(?P<pk>\d+)/$', login_required(CloudletDetailsView.as_view()), name='cloudlet-cloudlet-detail'),
)
