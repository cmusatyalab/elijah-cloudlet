
from django.conf.urls.defaults import patterns, url

from .views import IndexView, DetailView, LaunchInstanceView

INSTANCES = r'^(?P<instance_id>[^/]+)/%s$'
VIEW_MOD = 'openstack_dashboard.dashboards.project.instances.views'

urlpatterns = patterns('',
    url(r'^$', IndexView.as_view(), name='index'),
    url(r'^launch$', LaunchInstanceView.as_view(), name='launch'),
    #url(r'^(?P<instance_id>[^/]+)/$', DetailView.as_view(), name='detail'),
    #url(INSTANCES % 'update', UpdateView.as_view(), name='update'),
    #url(INSTANCES % 'console', 'console', name='console'),
    #url(INSTANCES % 'vnc', 'vnc', name='vnc'),
    #url(INSTANCES % 'spice', 'spice', name='spice'),
)
