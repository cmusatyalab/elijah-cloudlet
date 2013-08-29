from django.http import HttpResponse
from django.http import Http404
from django.shortcuts import render
from django.views import generic

from cloudlets.models import Cloudlet
from django.http import HttpResponseRedirect


def index(request):
    return render(request, "cloudlets/index.html", {})


class CloudletListView(generic.ListView):
    template_name = 'cloudlets/cloudlet_list.html'
    context_object_name = 'latest_cloudlet_list'

    def get_queryset(self):
        return Cloudlet.objects.order_by('-mod_time')


class CloudletDetailsView(generic.DetailView):
    model = Cloudlet
    template_name = 'cloudlets/cloudlet_details.html'


