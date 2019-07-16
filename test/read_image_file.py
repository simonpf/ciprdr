from IPython import get_ipython
ip = get_ipython()
if not ip is None:
    ip.magic("%load_ext autoreload")
    ip.magic("%autoreload 2")
import matplotlib.pyplot as plt
import ciprdr

image = ciprdr.PadsImageFile("img.cip")
image.set_timestamp_index(0)

c = 0
for i in range(10):
    img = image.get_particle_image()
    if img.data.shape[0] > 16:
        plt.matshow(img.data)
        plt.savefig("test_{}.png".format(c))
        c += 1
