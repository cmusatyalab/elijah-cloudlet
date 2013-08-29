from django.contrib import admin

from .models import *

class InvitationAdmin(admin.ModelAdmin):
    actions = ('resend',)
    list_display = ('email', 'invited_by', 'created', 'last_sent')
    search_fields = ('email',)

    def resend(self, request, queryset):
        for obj in queryset:
            obj.send(request)
    resend.short_description = 'Resend selected invitations'
admin.site.register(Invitation, InvitationAdmin)

class UserInfoAdmin(admin.ModelAdmin):
    search_fields = ('user__email',)
admin.site.register(UserInfo, UserInfoAdmin)
