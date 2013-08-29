from django.conf.urls import patterns, url
from django.core.urlresolvers import reverse_lazy

from .forms import (LoginForm, SignalingSetPasswordForm,
        SignalingPasswordChangeForm)
from .views import *

urlpatterns = patterns('',
    url(r'^profile/$', user_profile, name='cloudlet-profile'),
    url(r'^email/(.+)/$', change_email, name='cloudlet-change-email'),

    url(r'^login/$', 'django.contrib.auth.views.login', {
        'authentication_form': LoginForm,
    }, name='cloudlet-login'),
    url(r'^logout/$', 'django.contrib.auth.views.logout', {
        'next_page': reverse_lazy('cloudlet-home'),
    }, name='cloudlet-logout'),

    url(r'^invite/$', invite_user, name='cloudlet-invite'),
    url(r'^register/([a-z0-f]{40})/$', register, name='cloudlet-register'),

    url(r'^password/$', 'django.contrib.auth.views.password_change', {
        'password_change_form': SignalingPasswordChangeForm,
        'post_change_redirect': reverse_lazy('cloudlet-password-changed'),
    }, name='cloudlet-change-password'),
    url(r'^password/done/$', password_changed, name='cloudlet-password-changed'),

    url(r'^reset/$', 'django.contrib.auth.views.password_reset', {
        'post_reset_redirect': reverse_lazy('cloudlet-password-reset-sent'),
    }, name='cloudlet-reset-password'),
    url(r'^reset/sent/$', password_reset_sent, name='cloudlet-password-reset-sent'),
    url(r'^reset/(?P<uidb36>.+)/(?P<token>.+)/$', 'django.contrib.auth.views.password_reset_confirm', {
        'set_password_form': SignalingSetPasswordForm,
        'post_reset_redirect': reverse_lazy('cloudlet-password-reset-complete'),
    }),
    url(r'^reset/complete/$', password_reset_complete, name='cloudlet-password-reset-complete'),
)
