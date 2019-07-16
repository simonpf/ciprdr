////////////////////////////////////////////////////////////////////////////////
//
// C-bindings for the functions in cip.h.
//
// Author: Simon Pfreundschuh
//
////////////////////////////////////////////////////////////////////////////////
#include "cip.h"

#define DLL __attribute__ ((visibility ("default")))

////////////////////////////////////////////////////////////////////////////////
// Image file
////////////////////////////////////////////////////////////////////////////////

extern "C" {

    //
    // PadsImageFile
    //

    void * read_image_file(const char *filename_) {
        std::string filename(filename_);
        PadsImageFile *image = new PadsImageFile(filename);
        return image;
    }

    void destroy_image_file(PadsImageFile *image) {
        delete image;
    }

    TimeField set_timestamp_index(PadsImageFile *image, size_t index) {
        return image->set_timestamp_index(index);
    }

    //
    // PadsIndexFile
    //

    void * read_index_file(const char *filename_) {
        std::string filename(filename_);
        PadsIndexFile *index = new PadsIndexFile(filename);
        return index;
    }

    void destroy_index_file(PadsIndexFile *index) {
        delete index;
    }

    TimeField get_next_index(PadsIndexFile *index) {
        return index->next_index();
    }

    //
    // Particle images
    //

    ParticleImage * get_particle_image(PadsImageFile *image) {
        ParticleImage * pi = new ParticleImage(image->get_particle_image());
        return pi;
    }

    void destroy_particle_image(ParticleImage *image) {
        delete image;
    }
}
