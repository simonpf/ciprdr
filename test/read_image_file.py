from IPython import get_ipython
ip = get_ipython()
if not ip is None:
    ip.magic("%load_ext autoreload")
    ip.magic("%autoreload 2")

import ciprdr

image = ciprdr.PadsImage("../data/img.cip")
img = image.get_particle_image()
