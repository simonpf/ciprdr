from ciprdr.api import PadsImageFile, PadsIndexFile, ParticleImage
import os
import glob
import ctypes as c
import datetime
import numpy as np

from PIL import Image

class ImageFolder:
    def __init__(self, path):
        self.index_files = glob.glob(os.path.join(path, "Imageindex*"))
        self.index_files.sort()
        self.image_files = glob.glob(os.path.join(path, "Imagefile*"))
        self.image_files.sort()

    def _extract_images(self, image_index, timestamp_index, path):
        f = self.image_files[image_index]
        image = PadsImageFile(f)
        t = image.set_timestamp_index(timestamp_index).to_datetime()

        # Read all slices from timestamp
        images = []
        pi = image.get_particle_image()
        while(pi.struct.valid):
            data = np.array((255 / 3) * (3 - pi.data), dtype = np.uint8, copy = True)
            images += [data]
            pi = image.get_particle_image()


        if len(images) > 0:
            data = np.concatenate(images)
            print(data.shape)
            img = Image.fromarray(data.T, mode = "L")
            tt = (t.year, t.month, t.day, t.hour,
                  t.minute, t.second, t.microsecond // 1000)

            image_name = "cip_image_{0:04d}{1:02d}{2:02d}"
            image_name += "_{3:02d}{4:02d}{5:02d}_{6:03d}.png"
            image_name = image_name.format(*tt)

            img.save(os.path.join(path, image_name))
            print("saved image: ", image_name)

        del image, images



    def extract_images(self,
                       start_time,
                       end_time,
                       output_path = None,
                       regexp = None):

        #
        # Define fit predicate function.
        #
        if not regexp is None:
            regexp = re.compile(regexp)

        def matches(f):
            if regexp is None:
                return True
            return not regexp.match(f) is None

        if output_path is None:
            output_path = os.getcwd()

        #
        # Loop over all index files.
        #

        for i, f in enumerate(self.index_files):
            if matches(f):

                print("Opening index file {}".format(f))
                index = PadsIndexFile(f)

                # Loop over timestamps in index file.
                for j in range(index.n_timestamps):
                    ts = index.get_next_timestamp().to_datetime()
                    if (start_time <= ts) and (ts <= end_time):
                        self._extract_images(i, j, output_path)


