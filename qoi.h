#ifndef QOI_FORMAT_CODEC_QOI_H_
#define QOI_FORMAT_CODEC_QOI_H_

#include "utils.h"

constexpr uint8_t QOI_OP_INDEX_TAG = 0x00;
constexpr uint8_t QOI_OP_DIFF_TAG = 0x40;
constexpr uint8_t QOI_OP_LUMA_TAG = 0x80;
constexpr uint8_t QOI_OP_RUN_TAG = 0xc0;
constexpr uint8_t QOI_OP_RGB_TAG = 0xfe;
constexpr uint8_t QOI_OP_RGBA_TAG = 0xff;
constexpr uint8_t QOI_PADDING[8] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u};
constexpr uint8_t QOI_MASK_2 = 0xc0;

/**
 * @brief encode the raw pixel data of an image to qoi format.
 *
 * @param[in] width image width in pixels
 * @param[in] height image height in pixels
 * @param[in] channels number of color channels, 3 = RGB, 4 = RGBA
 * @param[in] colorspace image color space, 0 = sRGB with linear alpha, 1 = all channels linear
 *
 * @return bool true if it is a valid qoi format image, false otherwise
 */
bool QoiEncode(uint32_t width, uint32_t height, uint8_t channels, uint8_t colorspace = 0);

/**
 * @brief decode the qoi format of an image to raw pixel data
 *
 * @param[out] width image width in pixels
 * @param[out] height image height in pixels
 * @param[out] channels number of color channels, 3 = RGB, 4 = RGBA
 * @param[out] colorspace image color space, 0 = sRGB with linear alpha, 1 = all channels linear
 *
 * @return bool true if it is a valid qoi format image, false otherwise
 */
bool QoiDecode(uint32_t &width, uint32_t &height, uint8_t &channels, uint8_t &colorspace);

bool QoiEncode(uint32_t width, uint32_t height, uint8_t channels, uint8_t colorspace) {

    // qoi-header part

    // write magic bytes "qoif"
    QoiWriteChar('q');
    QoiWriteChar('o');
    QoiWriteChar('i');
    QoiWriteChar('f');
    // write image width
    QoiWriteU32(width);
    // write image height
    QoiWriteU32(height);
    // write channel number
    QoiWriteU8(channels);
    // write color space specifier
    QoiWriteU8(colorspace);

    /* qoi-data part */
    int run = 0;
    int px_num = width * height;

    uint8_t history[64][4];
    memset(history, 0, sizeof(history));

    uint8_t r, g, b, a;
    a = 255u;
    uint8_t pre_r, pre_g, pre_b, pre_a;
    pre_r = 0u;
    pre_g = 0u;
    pre_b = 0u;
    pre_a = 255u;

    for (int i = 0; i < px_num; ++i) {
        r = QoiReadU8();
        g = QoiReadU8();
        b = QoiReadU8();
        if (channels == 4)
            a = QoiReadU8();

        // TODO
        if (r == pre_r && g == pre_g && b == pre_b && a == pre_a) {
            run++;
            if (run == 62) {
                // if run size reach the limit (64-2), output all the run data and clear it
                QoiWriteU8(QOI_OP_RUN_TAG | (run - 1));
                run = 0;
            }
        } else {
            if (run > 0) {
                QoiWriteU8(QOI_OP_RUN_TAG | (run - 1));
                run = 0;
            }

            // get hash value
            int index_pos = QoiColorHash(r, g, b, a) % 64;

            if (history[index_pos][0] == r && history[index_pos][1] == g && history[index_pos][2] == b &&
                history[index_pos][3] == a) {
                // if the pixel is found in the history array, then it can be directly output.
                // here we use QoiWriteU8(uint_8) as the output function,
                // in which we should provide a number consists of two parts, two sign digits and six data digits.
                QoiWriteU8(QOI_OP_INDEX_TAG | index_pos);
            } else {
                // copy the pixel generated now into the history array
                history[index_pos][0] = r;
                history[index_pos][1] = g;
                history[index_pos][2] = b;
                history[index_pos][3] = a;

                // judge the mode of encoding
                if (a == pre_a) {
                    // calculate the difference between the pixel
                    int8_t dif_r = r - pre_r;
                    int8_t dif_g = g - pre_g;
                    int8_t dif_b = b - pre_b;

                    // calculate the difference between the dif
                    int8_t dif_rg = dif_r - dif_g;
                    int8_t dif_bg = dif_b - dif_g;

                    // noticing that the data type here is int8_t (signed char) instead of unsigned char,
                    // which would be easier to show the difference later.

                    if (dif_r >= -2 && dif_r <= 1 && dif_g >= -2 && dif_g <= 1 && dif_b >= -2 && dif_b <= 1) {
                        // if the difference of all color is small enough to be written in a single char
                        QoiWriteU8(QOI_OP_DIFF_TAG | (dif_r + 2) << 4 | (dif_g + 2) << 2 | (dif_b + 2));
                    } else if (dif_rg > -9 && dif_rg < 8 && dif_g > -33 && dif_g < 32 && dif_bg > -9 && dif_bg < 8) {
                        // if the difference is adequate to be written in two chars
                        QoiWriteU8(QOI_OP_LUMA_TAG | (dif_g + 32));
                        QoiWriteU8((dif_rg + 8) << 4 | (dif_bg + 8));
                    } else {
                        // or else we can only write the difference in three chars
                        QoiWriteU8(QOI_OP_RGB_TAG);
                        QoiWriteU8(r);
                        QoiWriteU8(g);
                        QoiWriteU8(b);
                    }
                } else {
                    // if it's the rgba mode
                    QoiWriteU8(QOI_OP_RGBA_TAG);
                    QoiWriteU8(r);
                    QoiWriteU8(g);
                    QoiWriteU8(b);
                    QoiWriteU8(a);
                }
            }
        }
        // update the pre array
        pre_r = r;
        pre_g = g;
        pre_b = b;
        pre_a = a;
    }

    // Remember to clear the last run, which I forgot at first
    // and find error when testing sample/rgba/dice.pam
    if (run > 0) {
        QoiWriteU8(QOI_OP_RUN_TAG | (run - 1));
        run = 0;
    }

    // qoi-padding part
    for (int i = 0; i < sizeof(QOI_PADDING) / sizeof(QOI_PADDING[0]); ++i) {
        QoiWriteU8(QOI_PADDING[i]);
    }

    return true;
}

bool QoiDecode(uint32_t &width, uint32_t &height, uint8_t &channels, uint8_t &colorspace) {

    char c1 = QoiReadChar();
    char c2 = QoiReadChar();
    char c3 = QoiReadChar();
    char c4 = QoiReadChar();
    if (c1 != 'q' || c2 != 'o' || c3 != 'i' || c4 != 'f') {
        return false;
    }

    // read image width
    width = QoiReadU32();
    // read image height
    height = QoiReadU32();
    // read channel number
    channels = QoiReadU8();
    // read color space specifier
    colorspace = QoiReadU8();

    int run = 0;
    int px_num = width * height;

    uint8_t history[64][4];
    memset(history, 0, sizeof(history));

    uint8_t r, g, b, a;
    a = 255u;

    for (int i = 0; i < px_num; ++i) {

        // TODO
        if (run) {
            // still continue running
            run--;
        } else {
            // end of a run or other pixels

            // first judge the sign of current pixel
            uint8_t sign = QoiReadU8();

            if (sign == QOI_OP_RGB_TAG) {
                // the sign of three independent differences
                r = QoiReadU8();
                g = QoiReadU8();
                b = QoiReadU8();
            } else if (sign == QOI_OP_RGBA_TAG) {
                // the sign of RGBA and four independent differeces
                r = QoiReadU8();
                g = QoiReadU8();
                b = QoiReadU8();
                a = QoiReadU8();
            } else if ((sign & QOI_MASK_2) == QOI_OP_INDEX_TAG) {
                // the sign of looking for index in the history array
                r = history[sign % 64][0];
                g = history[sign % 64][1];
                b = history[sign % 64][2];
                a = history[sign % 64][3];
            } else if ((sign & QOI_MASK_2) == QOI_OP_DIFF_TAG) {
                // the sign of having got the difference
                // calculate the actual pixel
                r += ((sign >> 4) & 0x03) - 2;
                g += ((sign >> 2) & 0x03) - 2;
                b += ((sign >> 0) & 0x03) - 2;
            } else if ((sign & QOI_MASK_2) == QOI_OP_LUMA_TAG) {
                // the sign of loading the difference from two numbers
                int signn = QoiReadU8();
                int dif_g = (sign & 0x3f) - 32;

                // calculate the actual pixel
                r += dif_g - 8 + ((signn >> 4) & 0x0f);
                g += dif_g;
                b += dif_g - 8 + ((signn >> 0) & 0x0f);
            } else if ((sign & QOI_MASK_2) == QOI_OP_RUN_TAG) {
                run = sign & 0x3f;
            }

            // record the current pixel in history array
            int index_pos = QoiColorHash(r, g, b, a);
            history[index_pos][0] = r;
            history[index_pos][1] = g;
            history[index_pos][2] = b;
            history[index_pos][3] = a;
        }
        
        QoiWriteU8(r);
        QoiWriteU8(g);
        QoiWriteU8(b);
        if (channels == 4)
            QoiWriteU8(a);
    }

    bool valid = true;
    for (int i = 0; i < sizeof(QOI_PADDING) / sizeof(QOI_PADDING[0]); ++i) {
        if (QoiReadU8() != QOI_PADDING[i])
            valid = false;
    }

    return valid;
}

#endif // QOI_FORMAT_CODEC_QOI_H_
