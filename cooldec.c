/*
 * Authors: Austin Stephens and Jonny Rallison
 * February 28, 2019
 * 
 * This is a .cool file decoder.  
 * 
 * This is patterned after the bmp.c decoder from ffmpeg.
 * Some sections have been taken from and adapted from the bmp decoder.  
 */

#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"

/*
 *  This perfoms the decoding of a .cool file image using the encoded information and displays the image
 * 
 *  avctx: AVContext - contains the information for the image
 *  data: Points to memory addresses in the AVFrame
 *  got_frame: Gets set to 1 before returning
 *  avpkt: Stores the buffer
 * 
 *  returns: returns an int so it knows if it successfully decoded or not
 */
static int cool_decode_frame(AVCodecContext *avctx,
                             void *data, int *got_frame,
                             AVPacket *avpkt)
{
    // These are the variables used to store data from the packet from the encoder
    const uint8_t *buf = avpkt->data;
    AVFrame *frame_pointer = data;
    int width, height;
    uint8_t *ptr;

    // Make sure that the file type is correct.  CL is for .cool files
    if (bytestream_get_byte(&buf) != 'C' ||
        bytestream_get_byte(&buf) != 'L')
    {
        av_log(avctx, AV_LOG_ERROR, "bad magic number\n");
        return AVERROR_INVALIDDATA;
    }

    // Get the height and the width of the image
    width = bytestream_get_le32(&buf);
    height = bytestream_get_le32(&buf);

    // Set they pixel format
    avctx->pix_fmt = AV_PIX_FMT_RGB8;

    // Set the dimensions of the image
    ff_set_dimensions(avctx, width, height);

    // Get the buffer for the image processing
    ff_get_buffer(avctx, frame_pointer, 0);

    // These are variables needed for the rest of ffmpeg
    frame_pointer->pict_type = AV_PICTURE_TYPE_I;
    frame_pointer->key_frame = 1;

    // Start the pointer at the end of the file and work your way back
    ptr = frame_pointer->data[0] + (avctx->height - 1) * frame_pointer->linesize[0];

    // Start the buffer 10 after the start because of the header information
    buf = avpkt->data + 10;

    // Copy the pixels from the file and put them on the screen line by line
    for (int i = 0; i < avctx->height; i++)
    {
        memcpy(ptr, buf, (int64_t)avctx->width);
        buf += (int64_t)avctx->width;
        ptr -= frame_pointer->linesize[0];
    }

    *got_frame = 1;

    return avpkt->size;
}

// Struct to decode the .cool files
AVCodec ff_cool_decoder = {
    .name = "cool",
    .long_name = NULL_IF_CONFIG_SMALL("Cool format"),
    .type = AVMEDIA_TYPE_VIDEO,
    .id = AV_CODEC_ID_COOL,
    .decode = cool_decode_frame,
    .capabilities = AV_CODEC_CAP_DR1,
};
