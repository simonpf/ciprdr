////////////////////////////////////////////////////////////////////////////////
//
// Functionality for decoding and parsing of CIP greyscale images.
//
// Author: Simon Pfreundschuh
//
////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <bitset>
#include <memory>
#include <vector>
#include <algorithm>

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

////////////////////////////////////////////////////////////////////////////////
// Greyscale decompression
////////////////////////////////////////////////////////////////////////////////

/** Stream of compressed CIP greyscale image data
 *
 * Objects of this class decompress a binary stream of CIP greyscale image
 * data.
 */
struct GreyscaleStream {

    /** State of the stream.
     *
     * The state of the greyscale stream is used as a marker to allow
     * going back in the byte stream.
     */
    struct State {
        size_t count = 0;
        char current_byte = 0;
        char last_two_bits = 0;
    };

    GreyscaleStream(std::fstream &input_)
        : input(&input_), limited(false)
    {
        // Nothing to do here.
    }

    GreyscaleStream(std::fstream &input_, size_t size_)
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

            if (limited) {
                --size;
            }

            input->read(reinterpret_cast<char *>(&current_byte), 1);
            if (input->eof()) {
                return 4;
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

    State get_marker() {
        return State{count, current_byte, last_two_bits};
    }

    void set_state(State m) {
        count = m.count;
        current_byte = m.current_byte;
        last_two_bits = m.last_two_bits;
    }

    void back() {
        std::bitset<8> bits(current_byte);
        size_t current_count = 0;
        ++count;

        if (bits[7]) {
            current_count = (current_byte & 0x7F);
        } else if (bits[6]) {
            current_count = 3;
        } else if (bits[4]) {
            current_count = 2;
        } else if (bits[2]) {
            current_count = 1;
        } else {
            current_count = 3;
        }

        if (count == current_count) {

            // Find non-zero byte

            input->unget();
            input->unget();
            input->read(reinterpret_cast<char *>(&current_byte), 1);

            while (current_byte == 0) {
               input->unget();
               input->unget();
               input->read(reinterpret_cast<char *>(&current_byte), 1);
            };

            // Now determine last two-bits of previous byte
            unsigned char previous_last_two = 0x80;
            size_t steps = 0;
            while (previous_last_two & 0x80) {
                std::bitset<8> bits2(previous_last_two);
                input->unget();
                input->unget();
                input->read(reinterpret_cast<char *>(&previous_last_two), 1);
                ++steps;
            };

            // Now get back to previous position.
            for (size_t i = 0; i < steps; ++i) {
                input->get();
            }

            last_two_bits = previous_last_two & 0x3;
            bits = std::bitset<8>(current_byte);

            count = 0;
        }
    }

    void back(size_t n) {
        for (size_t i = 0; i < n; ++i) {
            back();
        }
    }

    std::vector<char> peek(size_t n) {
        size_t count_prev = count;
        size_t pos = input->tellg();
        char current_byte_prev = current_byte;
        char last_two_bits_prev = last_two_bits;

        std::vector<char> data(n);
        for (size_t i = 0; i < n; ++i) {
            data[i] = decompress();
        }

        input->seekg(pos);
        count = count_prev;
        current_byte = current_byte_prev;
        last_two_bits = last_two_bits_prev;

        return data;
    }

    std::fstream * input = nullptr;
    size_t count = 0;
    char current_byte = 0;
    char last_two_bits = 0;

    size_t size = 0;
    bool limited = false;

};

////////////////////////////////////////////////////////////////////////////////
// Particle images
////////////////////////////////////////////////////////////////////////////////

/** CIP particle image read from GreyscaleStream.
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
    bool valid = true;
    char header[64];
    char *image = nullptr;

    /**
     * Read particle image from GreyscaleStream object.
     */
    ParticleImage(GreyscaleStream &gs) {

        // Read the header.
        // Note that bits need not be reversed due
        find_next_particle_boundary(gs);
        gs.back();

        for (size_t i = 0; i < 64; ++i) {
            // Read byte and check for validity.
            unsigned char in = gs.decompress();
            if (in > 3) {
                valid = false;
                slices = 0;
                return;
            }
            header[i] = in;
        }

        // Decode particle header.
        v_air = get_number(56, 63);
        count = get_number(64, 79);
        microseconds = get_number(83, 92);
        milliseconds = get_number(93, 102);
        seconds = get_number(103, 108);
        minutes = get_number(109, 114);
        hours = get_number(115, 119);
        slices = get_number(120, 127);

        // Read slices.
        // Stop if more than 128 threes are encountered.
        size_t threes = 0;
        size_t slice_counter = 0;
        GreyscaleStream::State marker = gs.get_marker();
        std::cout << "Reading " << slices << " slices." << std::endl;
        if (slices > 1) {
            image = new char[(slices - 1) * 64];
            // Initialize image data to 0.
            for (size_t i = 0; i < (slices - 1) * 64; ++i) {
                image[i] = 3;
            }
            // Read image slices
            for (size_t i = 0; i < (slices - 1) * 64; ++i) {
                // Read byte and check for validity.
                slice_counter = i / 64;
                unsigned char in = gs.decompress();
                if (in > 3) {
                    valid = false;
                    return;
                }
                image[i] = in;

                if (in == 3) {
                    threes++;
                } else if (threes > 127) {
                    if (in == 0) {
                        auto data = gs.peek(27);
                        auto pred = [](char x){return x == 0;};
                        if (std::all_of(data.begin(), data.end(), pred)) {
                            gs.set_state(marker);
                            break;
                        }
                    }
                } else {
                    threes = 0;
                    marker = gs.get_marker();
                }
            }
        }
        slices = slice_counter + 1;
    }

    ParticleImage(ParticleImage &&other) {
       v_air = other.v_air;
       count = other.count;
       microseconds = other.microseconds;
       milliseconds = other.milliseconds;
       seconds = other.seconds;
       minutes = other.minutes;
       hours = other.hours;
       slices = other.slices;
       valid = other.valid;

       std::copy(other.header, other.header + 64, header);
       image = other.image;
       other.image = nullptr;
    }

    ParticleImage(const ParticleImage &)  = delete;
    ParticleImage& operator=(const ParticleImage)    = delete;
    ParticleImage& operator=(      ParticleImage &&) = delete;

    ~ParticleImage() {
        if (image) {
            delete[] image;
        }
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
    static void find_next_particle_boundary(GreyscaleStream &gs) {
        long threes = 0;
        long counter = 1;
        char in = 0;

        while (threes < 64) {

            // Get byte and check validity.
            in = gs.decompress();
            if (in > 3) {
                return;
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
            counter++;
        }
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
    GreyscaleStream gs;

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
        gs = GreyscaleStream(file, 4096);
        return tf;
    }

    ParticleImage get_particle_image() {
        ParticleImage pi(gs);
        return pi;
    }

};

struct PadsIndexFile {

    size_t n_timestamps = 0;
    std::fstream file;

    PadsIndexFile(std::string filename)
    : file(filename, std::fstream::in | std::fstream::binary)
    {
        file.seekg (0, file.end);
        size_t length = file.tellg();
        file.seekg (0, file.beg);

        n_timestamps = length / sizeof(TimeField);
    }

    TimeField next_index() {
        TimeField ind;
        file.read(reinterpret_cast<char *>(&ind), sizeof(ind));
        return ind;
    }

};
