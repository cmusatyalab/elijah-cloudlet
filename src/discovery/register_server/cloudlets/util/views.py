from django.shortcuts import render

def land_message(request, header, body):
    return render(request, 'util/message.html', {
        'header': header,
        'body': body,
    })
