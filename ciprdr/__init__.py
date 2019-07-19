"""
ciprdr
======

This module implements the user interface for the ciprdr python package.
It provides functionality for extracting particle images from CIP raw
data.
"""
import ctypes as c
import datetime
import glob
import numpy as np
import os

from PIL import Image
from ciprdr.api import PadsImageFile, PadsIndexFile, ParticleImage


class ImageFolder:
    """
    The image folder class parses the content of a folder for filenames that
    start with ImageIndex and Imagefile and contain image data from CIP probes.
    The ImageFolder object can then be used to extract images of the particles
    from the raw data.
    """
    def __init__(self, path):
        self.index_files = glob.glob(os.path.join(path, "Imageindex*"))
        self.index_files.sort()
        self.image_files = glob.glob(os.path.join(path, "Imagefile*"))
        self.image_files.sort()

    def _extract_combined_images(self, image_index, timestamp_index, path):
        """
        Combines stored particles from a timestamp into a single image. Since small particles
        are combined into larger images, this is probably the best to obtain an overview over
        the data.
        """
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


    def extract_combined_images(self,
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
                        self._extract_combined_images(i, j, output_path)


    def _extract_single_images(self,
                               image_index,
                               timestamp_index,
                               path,
                               output_size = None):
        """
        Combines stored particles from a timestamp into a single image.
        """
        f = self.image_files[image_index]
        image = PadsImageFile(f)
        t = image.set_timestamp_index(timestamp_index).to_datetime()

        # Read all slices from timestamp
        c = 0
        pi = image.get_particle_image()
        while(pi.struct.valid):
            data = np.array((255 / 3) * (3 - pi.data), dtype = np.uint8, copy = True)

            # Extract individual particles and resize if necessary
            labels = sp.ndimage.label(data > 0)[0]
            bbs = sp.ndimage.find_objects(lables)
            for bb in bbs:
                max_width = max([s.stop - s.start for s in bb])
                image = np.zeros((max_width, max_width))
                center = max_width // 2

                target_slices = (,)
                for s in bb:
                    extent = s.stop - s.start
                    left = center - extent // 2
                    right = left + extent
                    target_slices += (slice(left, right),)

                image[target_slices] = image

                if not output_size is None:
                    image = sp.ndimage.imresize(output_size)

                # Save the image
                image = Image.fromarray(image.T, mode = "L")
                tt = (t.year, t.month, t.day, t.hour,
                      t.minute, t.second, t.microsecond // 1000,
                      c)
                image_name = "cip_image_{0:04d}{1:02d}{2:02d}"
                image_name += "_{3:02d}{4:02d}{5:02d}_{6:03d}_{7:04d}.png"
                image_name = image_name.format(*tt)
                c += 0

                image.save(os.path.join(path, image_name))
        del image, images

    def extract_combined_images(self,
                                start_time,
                                end_time,
                                output_path = None,
                                regexp = None,
                                output_size = None):

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
                        self._extract_singe_images(i, j, output_path_output_size)
