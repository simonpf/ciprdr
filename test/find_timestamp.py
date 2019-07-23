from IPython import get_ipython
ip = get_ipython()
if not ip is None:
    ip.magic("%load_ext autoreload")
    ip.magic("%autoreload 2")
import ciprdr
import datetime

t = datetime.datetime(2016, 10, 14, 10, 37, 12, microsecond = 2000)
t1 = datetime.datetime(2016, 10, 14, 11, 37, 12, microsecond = 2001)
images = ciprdr.ImageFolder("/home/simonpf/src/joint_flight/data/particle_images")
tf = images.find_timestamp(t)
