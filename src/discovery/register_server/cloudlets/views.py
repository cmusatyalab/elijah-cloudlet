from django.http import HttpResponse
from django.http import Http404
from django.shortcuts import render
from django.contrib.auth import authenticate
from django.views import generic

from cloudlets.models import Cloudlet

class IndexView(generic.ListView):
    template_name = 'cloudlets/index.html'
    context_object_name = 'latest_cloudlet_list'

    def get_queryset(self):
        import pdb;pdb.set_trace()
        user = authenticate(username='john', password='secret')
        if user is not None:
            if user.is_active:
                return "success"
            else:
                return "inactivated"
        else:
            return "failed"
        return Cloudlet.objects.order_by('-mod_time')

class DetailsView(generic.DetailView):
    model = Cloudlet
    template_name = 'cloudlets/details.html'


