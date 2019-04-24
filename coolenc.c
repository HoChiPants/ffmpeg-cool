/*
 * Authors: Austin Stephens and Jonny Rallison
 * February 28, 2019
 * 
 * This is an encoder for a .cool file type.
 * The layout of this encoder was patterned after ffmpeg's bmp encoder (bmpenc.c) - 
 * Some sections have been taken from and modified from the bmp encoder.
 */

#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"

/*
 * Sets the bits_per_coded_sample (to 8) if the format is right, 
 * Throws an error if it's an unsupported pixel format
 * 
 * avctx: Hold information about the image
 * 
 * returns: An error message or 0
 */
static av_cold int cool_encode_init(AVCodecContext *avctx)
{
    if (avctx->pix_fmt == AV_PIX_FMT_RGB8)
        avctx->bits_per_coded_sample = 8;
    else
    {
        av_log(avctx, AV_LOG_INFO, "unsupported pixel format\n");
        return AVERROR(EINVAL);
    }
    return 0;
}

/*
 * Encodes the picture, pixel-by-pixel, in a way that lets the decoder later on read it and print the picture.
 * 
 * This is accomplished by getting the size of the image (width * height) in bytes and allocating enough memory to encode information
 * for each pixel in the image.
 * 
 * avctx: AVContext - contains all the information about the image (i.e. dimensions)
 * pkt: Used for the buffer
 * pict: The frame pointer is set to this
 * got_packet: Gets set to 1
 * 
 * returns: returns an int to let what called this method know if it encoded correctly
 */
static int cool_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
                             const AVFrame *pict, int *got_packet)
{
    // Variables to store information to pass to the decoder
    const AVFrame *const frame_pointer = pict;
    uint8_t *ptr, *buf;

// This is information that the ffmpeg needs for later use
#if FF_API_CODED_FRAME
    FF_DISABLE_DEPRECATION_WARNINGS
    avctx->coded_frame->pict_type = AV_PICTURE_TYPE_I;
    avctx->coded_frame->key_frame = 1;
    FF_ENABLE_DEPRECATION_WARNINGS
#endif

    // Stores the total number of bytes (height*width) since it's 1 byte per pixel
    int n_bytes = avctx->height * avctx->width;

    // Allocate the correct amount of memory for the buffer reader
    ff_alloc_packet2(avctx, pkt, n_bytes + 10, 0);

    // Start the pointer at the begining of the data in memory
    buf = pkt->data;

    // Load the header information
    bytestream_put_byte(&buf, 'C');                 // magic letter
    bytestream_put_byte(&buf, 'L');                 // magic letter
    bytestream_put_le32(&buf, avctx->width);        // width
    bytestream_put_le32(&buf, avctx->height);       // height

    // Encode the data into the buffer pointer from bottom to top
    ptr = frame_pointer->data[0] + (avctx->height - 1) * frame_pointer->linesize[0];
    buf = pkt->data + 10;
    for (int i = 0; i < avctx->height; i++)
    {
        memcpy(buf, ptr, (int64_t)avctx->width);
        buf += (int64_t)avctx->width;
        ptr -= frame_pointer->linesize[0]; // ... and go back
    }

    //Necessary information for ffmpeg
    pkt->flags |= AV_PKT_FLAG_KEY;
    *got_packet = 1;
    return 0;
}

/*
 * A struct to start the .cool file type encoder
 */
AVCodec ff_cool_encoder = {
    .name = "cool",
    .long_name = NULL_IF_CONFIG_SMALL("Cool format"),
    .type = AVMEDIA_TYPE_VIDEO,
    .id = AV_CODEC_ID_COOL,
    .init = cool_encode_init,
    .encode2 = cool_encode_frame,
    .pix_fmts = (const enum AVPixelFormat[]){
        AV_PIX_FMT_RGB8,
        AV_PIX_FMT_NONE},
};
