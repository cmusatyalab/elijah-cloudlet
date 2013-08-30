from django.contrib.auth.decorators import login_required
from django.shortcuts import render


def basevm_list(request):
    return render(request, "basevm_list.html", {})
