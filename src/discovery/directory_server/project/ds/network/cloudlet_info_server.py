#!/usr/bin/env python

#from flask import Flask,request,render_template, Response
from flask import Flask
from cloudlet_info_const import Const as Const
from pprint import pprint
import json
import libvirt

app = Flask(__name__)
app.config.from_object(__name__)

DEBUG = True


@app.route("/machine/")
def show_machine():
    machine_info = _get_machine_info()
    pprint(machine_info)
    json_ret = json.dumps(machine_info)
    return json_ret


def _get_machine_info():
    # libvirt infomation
    conn = libvirt.open("qemu:///session")
    machine_dict = dict()
    machine_info = conn.getInfo()
    machine_dict[Const.MACHINE_MEM_TOTAL] = machine_info[1]
    machine_dict[Const.MACHINE_CLOCK_SPEED] = machine_info[3]
    machine_dict[Const.MACHINE_NUMBER_SOCKET] = machine_info[5]
    machine_dict[Const.MACHINE_NUMBER_CORES] = machine_info[6]
    machine_dict[Const.MACHINE_NUMBER_THREADS_PCORE] = machine_info[7]

    return machine_dict

	
if __name__ == "__main__":
    pprint(_get_machine_info())
    app.run(host='0.0.0.0')
