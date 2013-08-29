from django.conf.urls import patterns, url, include
from cloudlets import views

urlpatterns = patterns('',
    # ex: /cloudlets/
    url(r'^$', views.CloudletListView.as_view(), name='cloudlet-list'),
    # Ex: /cloudlets/5/
    url(r'^(?P<pk>\d+)/$', views.CloudletDetailsView.as_view(), name='cloudlet-detail'),
)
