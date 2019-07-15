#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include "cip.h"

#define DLL __attribute__ ((visibility ("default")))

////////////////////////////////////////////////////////////////////////////////
// Image file
////////////////////////////////////////////////////////////////////////////////

extern "C" {

    void * read_image_file(const char *filename_) {
        std::string filename(filename_);
        PadsImageFile *image = new PadsImageFile(filename);
        std::cout << "Creating image:" << image << std::endl;
        return image;
    }

    void destroy_image_file(PadsImageFile *image) {
        delete image;
    }

    TimeField set_timestamp_index(PadsImageFile *image, size_t index) {
        std::cout << "Setting timestamp: " << image << " // " << index << std::endl;
        return image->set_timestamp_index(index);
    }

    ParticleImage * get_particle_image(PadsImageFile *image) {
        ParticleImage * pi = new ParticleImage(image->get_particle_image());
        std::cout << pi->hours << " / " << pi->minutes << std::endl;
        return pi;
    }

    void destroy_particle_image(ParticleImage *image) {
        delete image;
    }
}

