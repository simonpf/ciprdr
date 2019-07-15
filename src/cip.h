#include <iostream>
#include <bitset>
#include <memory>
#include <utility>

struct TimeField {
    short year;
    short month;
    short day;
    short hour;
    short minute;
    short second;
    short milliseconds;
    short weekday;
};

struct IndexField {
    TimeField time;
    long offset;
};

////////////////////////////////////////////////////////////////////////////////
// Greyscale decompression
////////////////////////////////////////////////////////////////////////////////

struct GreyScaleStream {

    GreyScaleStream(std::fstream &input_)
        : input(&input_), limited(false)
    {
        // Nothing to do here.
    }

    GreyScaleStream(std::fstream &input_, size_t size_)
        : input(&input_), size(size_), limited(true)
        {
            // Nothing to do here.
        }

    char decompress() {

        std::bitset<8> bits(current_byte);

        if (count == 0) {
            // Read byte and decide what to do.

            if (limited && (size == 0)) {
                return 4;
            }
            input->read(reinterpret_cast<char *>(&current_byte), 1);
            if (limited) {
                size--;
            }

            bits = std::bitset<8>(current_byte);

            if (bits[7]) {
                // First bit is one: byte is counter byte
                count = (current_byte & 0x7F);
            } else if (bits[6]) {
                count = 3;
            } else if (bits[4]) {
                count = 2;
            } else if (bits[2]) {
                count = 1;
            } else {
                return decompress();
            }
        }

        --count;
        if (bits[7]) {
            return last_two_bits;
        } else {
            last_two_bits = (current_byte & (0x3 << (2 * count))) >> (2 * count);
            return last_two_bits;
        }
    }


    std::fstream * input;
    size_t count = 0;
    char current_byte = 0;
    char last_two_bits = 0;

    size_t size = 0;
    bool limited = false;

};

////////////////////////////////////////////////////////////////////////////////
// Particle images
////////////////////////////////////////////////////////////////////////////////

/** CIP particle image read from GreyScaleStream.
 *
 * Objects of this class represent particle images that are read from a
 * CIP greyscale image stream. The image consists of a 64-byte header slice,
 * followed by a variable number of image slices. Trailer slices are not
 * included in the data.
 *
 */
struct ParticleImage {


    size_t v_air, count, microseconds, milliseconds, seconds, minutes, hours,
        slices;
    bool valid;
    char header[64];
    char *image;

    /**
     * Read particle image from GreyscaleStream object.
     */
    ParticleImage(GreyScaleStream &gs) {

        // Read the header.
        // Note that bits need not be reversed due
        unsigned char first = next_particle_boundary(gs);
        if (first > 3) {
            valid = false;
            return;
        }
        header[0] = first;

        for (size_t i = 1; i < 64; ++i) {

            // Read byte and check for validity.
            unsigned char in = gs.decompress();
            if (in > 3) {
                valid = false;
                return;
            }
            header[i] = in;
        }

        v_air = get_number(56, 63);
        count = get_number(64, 79);
        microseconds = get_number(83, 92);
        milliseconds = get_number(93, 102);
        seconds = get_number(103, 108);
        minutes = get_number(109, 114);
        hours = get_number(115, 119);
        slices = get_number(120, 127);

        std::cout << "Reading " << slices << " sclices. " << std::endl;

        image = new char[(slices - 1) * 64];

        // Read image slices
        for (size_t i = 0; i < (slices - 1) * 64; i++) {
            // Read byte and check for validity.
            unsigned char in = gs.decompress();
            if (in > 3) {
                valid = false;
                return;
            }
            image[i] = in;
        }
    }

    ~ParticleImage() {
        delete image;
    }

    /** Return bit from header.
     *
     * Returns a given bit from the 128-bit particle header as described
     * in the CIP data format documentation. No pairwise reversal of bit
     * order is required on the returned bits.
     *
     * @i The index of the bit (0 - 127)
     */
    char get_header_bit(size_t i) {
        if ((i % 2) == 0) {
            return header[i / 2] & 1;
        } else {
            return header[i / 2] / 2;
        }
    }

    /** Interpret header bits as number.
     *
     * Returns the number represented by the bits at indices lsb_index - msb_index
     * in the particle header.
     */
    size_t get_number(size_t lsb_index, size_t msb_index) {
        size_t result = 0;
        for (size_t i = msb_index; i >= lsb_index; --i) {
            result *= 2;
            result += get_header_bit(i);
        }
        return result;
    }

    /**
     * Find start of next particle.
     */
    static char next_particle_boundary(GreyScaleStream &gs) {
        long threes = 0;
        long counter = 1;
        char in = 0;

        while (threes < 128) {

            // Get byte and check validity.
            in = gs.decompress();
            if (in > 3) {
                return 4;
            }

            counter++;
            if (in == 3) {
                ++threes;
            } else {
                threes = 0;
            }
        }

        // Ignore consecutive ones.
        while (in == 3) {
            in = gs.decompress();
        }
        std::cout << "Skipped " << counter << "." << std::endl;
        return in;
    }

    bool check() {
        // Check if full header was read.
        if (!valid) {
            return false;
        }

        // Check if first 28 bytes are zero.
        for (size_t i = 0; i < 56; ++i) {
            if (get_header_bit(i) != 0) return false;
        }
        return true;
    }

};

////////////////////////////////////////////////////////////////////////////////
// PADS greyscale image file
////////////////////////////////////////////////////////////////////////////////

struct PadsImageFile {

    size_t n_timestamps = 0;
    size_t timestamp_index = 0;
    std::fstream file;
    GreyScaleStream gs;

    PadsImageFile(std::string filename)
        : file(filename, std::fstream::in | std::fstream::binary),
          gs(file)
        {
            file.seekg (0, file.end);
            size_t length = file.tellg();
            file.seekg (0, file.beg);

            n_timestamps = length / 4112;
            file = std::fstream(filename, std::fstream::in | std::fstream::binary);
        }

    TimeField set_timestamp_index(size_t index) {
        file.seekg(index * 4112, file.beg);
        TimeField tf;
        file.read(reinterpret_cast<char *>(&tf), sizeof(tf));
        gs = GreyScaleStream(gs);
        return tf;
    }

    ParticleImage get_particle_image() {
        ParticleImage pi(gs);
        return pi;
    }

};
