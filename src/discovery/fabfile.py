from fabric.api import *
from fabric.contrib import files

django_dir = "./cloudlet/src/discovery/register_server"

@hosts('krha@rain.elijah.cs.cmu.edu')
def deploy():
    local('git push origin master --tags')

    if not files.exists('.pip/config'):
        run('mkdir -p .pip')
        files.append('.pip/config', ['[install]','download-cache=$HOME/.pip/cache'])

    if not files.exists(django_dir):
        msg = "current dir : %s\n" % str(run('pwd'))
        msg += "failed to find register_server directory\n"
        msg += "please download from the git"
        abort(msg)


    with cd(django_dir):
        run('virtualenv --system-site-packages env')
        run('touch requirements.txt')
        run('git pull')

        # Update virtualenv
        with settings(hide('running', 'warnings'), warn_only=True):
            update_env = run('test requirements.txt -ot env').failed

        if update_env:
            run('. env/bin/pip install -q -r requirements.txt')
            run('touch env')

        # Install node if necessary
        if not files.exists('env/bin/node'):
            run('. env/bin/activate && nodeenv -p')
            run('touch requirements-node.txt')

        # Update node
        with settings(hide('running', 'warnings'), warn_only=True):
            update_node = run('test requirements-node.txt -ot env/lib/node_modules').failed

        if update_node:
            run('. env/bin/activate && npm install -g $(cat requirements-node.txt)')
            run('touch env/lib/node_modules')

        # remove .pyc files
        run('find . -name env -prune -o -name \*.pyc -exec rm {} \;')

