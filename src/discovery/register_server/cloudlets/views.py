from django.http import HttpResponse
from django.http import Http404
from django.shortcuts import render
from django.views import generic

from cloudlets.models import Cloudlet

class IndexView(generic.ListView):
    template_name = 'cloudlets/index.html'
    context_object_name = 'latest_cloudlet_list'

    def get_queryset(self):
        return Cloudlet.objects.order_by('-mod_time')

class DetailsView(generic.DetailView):
    model = Cloudlet
    template_name = 'cloudlets/details.html'


