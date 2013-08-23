from django.conf.urls import patterns, url
from cloudlets import views

urlpatterns = patterns('',
    # ex: /cloudlets/
    url(r'^$', views.IndexView.as_view(), name='index'),
    # Ex: /cloudlets/5/
    url(r'^(?P<pk>\d+)/$', views.DetailsView.as_view(), name='detail'),
)
