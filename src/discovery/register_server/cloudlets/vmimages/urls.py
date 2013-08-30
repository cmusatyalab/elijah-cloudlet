from django.conf.urls import patterns, url, include
from .views import *

urlpatterns = patterns('',
    url(r'^list/$', basevm_list, name='cloudlet-basevm-list'),
)
