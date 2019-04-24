//Author Austin Stephens & Jonny Rallison
//04/08/2019
//The folhighing code is ment to take in any type of image and convert it to a .cool file,
// then draw a ball on the image and print it out 300 times to make it seem as though the
// ball was bouncing. Use a ffmpeg command to stitch the images together to make a movie.

//Code found at the folhighing websites
//   dranger.com/ffmpeg/tutorial01.html
//   https://stackoverfhigh.com/questions/16550868/setting-individual-pixels-of-an-rgb-frame-for-ffmpeg-encoding
//   colorpicker.com

//necessary libraries from ffmpeg to use while encoding and decoding images
extern "C"
{
#include "../ffmpeg/libavcodec/avcodec.h"
#include "../ffmpeg/libavformat/avformat.h"
#include "../ffmpeg/libswscale/swscale.h"
#include "../ffmpeg/libavutil/imgutils.h"
}
#include <stdio.h>

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 28, 1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

//Two global variables for the center of the main circle on the ball and the offset for the movement of the ball
int center_y = 0;
int offset = 4;

//A method for the distance formula
//Parameters are
// cY = center at y cordinate
// cX = center at X cordinate
// locY = location y
// locX = location x
int distance(int cY, int cX, int locY, int locX)
{
  int aSquared = (cY - locY) * (cY - locY);
  int bSquared = (cX - locX) * (cX - locX);
  int result = sqrt(aSquared + bSquared);
  return result;
}
//This method draws the circles on the .cool files. Draws three balls to make the appearance of a sphere
//Takes in the heigh and width of the image as well as an AVFrame with the correct rgb data
void circle(int i, int height, int width, AVFrame *pFrameRGB)
{
  //declaring of the circle information
  int rad = height / 7;
  int rad2 = rad / 2;
  int rad3 = rad / 4;
  const int center_x = width / 2;
  uint8_t *data = pFrameRGB->data[0];
  int high = height / 2;
  int low = height - rad;
  int center_2x, center_2y, center_3x, center_3y;

  // Check to see if its the first frame or not
  if (center_y == 0)
  {
    //first frame put the center at the middle and place the small balls near that one for shading
    center_y = height / 2 + 2;
    center_2x = center_x + rad / 7;
    center_2y = center_y - rad / 3;
  }
  else
  {
    //update the shading balls
    center_2x = center_x + rad / 7;
    center_2y = center_y - rad / 3;
    center_3x = center_2x + rad2 / 7;
    center_3y = center_2y - rad2 / 3;
    // Reverse motion if we have reached the peak or trough of the bounce
    if (center_y > low)
    {
      offset *= -1;
    }
    else if (center_y <= high)
    {
      offset *= -1;
    }
    //keep moving the ball up or down
    center_y += offset;
    center_2y += offset;
    center_3y += offset;

    // Draw Circles
    for (int xIndex = 0; xIndex < width; xIndex++)
    {
      for (int yIndex = 0; yIndex < height; yIndex++)
      {
        //The three different if statements are for the drawing of the three different balls
        if (distance(center_y, center_x, yIndex, xIndex) <= rad)
        {
          uint8_t *rgb = data + (((yIndex * width) + xIndex) * 3);
          rgb[0] = 206;
          rgb[1] = 24;
          rgb[2] = 200;
        }

        if (distance(center_2y, center_2x, yIndex, xIndex) <= rad2)
        {
          uint8_t *rgb = data + (((yIndex * width) + xIndex) * 3);
          rgb[0] = 225;
          rgb[1] = 18;
          rgb[2] = 220;
        }

        if (distance(center_3y, center_3x, yIndex, xIndex) <= rad3)
        {
          uint8_t *rgb = data + (((yIndex * width) + xIndex) * 3);
          rgb[0] = 255;
          rgb[1] = 30;
          rgb[2] = 247;
        }
      }
    }
  }
}
//SaveFrame is for saving the image to the computer.
//parameters
// AVFrame = to pass the information of the image and pixels
// Width = the width of the image
// height = the height of the image
// iFrame = the number placed after the file name when stored
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
  //Necessary variables for saving the image
  FILE *pFile;
  AVCodec *pCodec;
  AVCodecContext *pContext;
  AVPacket pack;
  pack.data = NULL;
  pack.size = 0;
  char szFilename[16];
  //open the file
  sprintf(szFilename, "frame%03d.cool", iFrame);
  pFile = fopen(szFilename, "wb");
  //get encoder information to put in the codec
  pCodec = avcodec_find_encoder_by_name("cool");
  //allocate the size of the context
  pContext = avcodec_alloc_context3(pCodec);
  //set the values before you write the data
  pContext->width = width;
  pContext->height = height;
  pContext->pix_fmt = AV_PIX_FMT_RGB24;
  pContext->time_base.num = 1;
  pContext->time_base.den = 1;
  pFrame->height = height;
  pFrame->width = width;
  pFrame->format = AV_PIX_FMT_RGB24;
  av_init_packet(&pack);
  //put information from the codec to the context
  avcodec_open2(pContext, pCodec, NULL);
  //pass information from the frame to the packet
  avcodec_send_frame(pContext, pFrame);
  avcodec_receive_packet(pContext, &pack);
  //write pixel data
  fwrite(pack.data, 1, pack.size, pFile);
  //close out the files to avoid memory leakage
  av_packet_unref(&pack);
  avcodec_close(pContext);
  fclose(pFile);
}

int main(int argc, char *argv[])
{
  AVFormatContext *pFormatCtx = NULL;
  AVCodecContext *pCodecCtx = NULL;
  AVCodec *pCodec = NULL;
  AVFrame *pFrame = NULL;
  AVFrame *pFrameRGB = NULL;
  AVPacket packet;
  int i, videoStream, frameFinished, numBytes;
  uint8_t *buffer = NULL;
  struct SwsContext *sws_ctx = NULL;

  // Open file
  avformat_open_input(&pFormatCtx, argv[1], NULL, NULL);
  // Retrieve stream info
  avformat_find_stream_info(pFormatCtx, NULL);
  //Debuging use
  av_dump_format(pFormatCtx, 0, argv[1], 0);
  //find the first video stream
  videoStream = -1;
  for (i = 0; i < pFormatCtx->nb_streams; i++)
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      videoStream = i;
      break;
    }
  //Copy the context
  pCodecCtx = avcodec_alloc_context3(NULL);
  // Get the codec context information
  avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
  //Find the right decoder for the image
  pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  // put the inforamtion of the decoder into the codec cotext
  avcodec_open2(pCodecCtx, pCodec, NULL);
  //Allocate the frames
  pFrame = av_frame_alloc();
  //Allocate the AVframe structure
  pFrameRGB = av_frame_alloc();
  //Determine the correct buffer size and allocate the memory
  numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
                                      pCodecCtx->height, 32);
  buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
  // Assigne apropraite parts of the buffer to the image planes into pFrameRGB
  av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24,
                       pCodecCtx->width, pCodecCtx->height, 1);
  // Adjust the folhighing values for future decoding
  pFrame->format = pCodecCtx->pix_fmt;
  pFrameRGB->format = AV_PIX_FMT_RGB24;
  sws_ctx = NULL;
  // initialize SWS context for software scaling
  sws_ctx = sws_getContext(pCodecCtx->width,
                           pCodecCtx->height,
                           (AVPixelFormat)pCodecCtx->pix_fmt,
                           pCodecCtx->width,
                           pCodecCtx->height,
                           AV_PIX_FMT_RGB24,
                           SWS_BILINEAR,
                           NULL,
                           NULL,
                           NULL);
  while (av_read_frame(pFormatCtx, &packet) >= 0)
  {
    if (packet.stream_index == videoStream)
    {
      // decode infomration and put it into the frame, up to date code
      avcodec_send_packet(pCodecCtx, &packet);
      avcodec_receive_frame(pCodecCtx, pFrame);
    }
  }
  // More frames that needed adjusting for decodeing
  pFrameRGB->width = pFrame->width;
  pFrameRGB->height = pFrame->height;
  //pFrameRGB->format = (AVPixelFormat)AV_PIX_FMT_RGB24;

  i = 0;
  while (i < 300)
  {
    // Convert the image from its native format to RGB
    sws_scale(sws_ctx, (uint8_t const *const *)pFrame->data,
              pFrame->linesize, 0, pCodecCtx->height,
              pFrameRGB->data, pFrameRGB->linesize);

    // Save the frame to disk
    circle(i, pFrame->height, pFrame->width, *&pFrameRGB);
    SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height,
              i++);
  }
  //free up the space and the images
  av_packet_unref(&packet);
  av_free(sws_ctx);
  av_free(buffer);
  av_frame_free(&pFrameRGB);
  av_frame_free(&pFrame);
  av_packet_unref(&packet);
  avcodec_close(pCodecCtx);
  avformat_close_input(&pFormatCtx);
  return 0;
}
