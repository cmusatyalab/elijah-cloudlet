from django.dispatch import Signal

# Best-effort signal: will trigger on our own views, but will not trigger
# if e.g. the password is changed through the admin.  Note that we can't
# implement this by catching pre_save on the User model, because automatic
# hash upgrades may update the password field even if the password itself
# doesn't change.
password_changed = Signal(providing_args=['user'])
