#include "H264BitStreamParser.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sstream>
#include <string>
#include <iostream>

// ffmpeg -i in.264 -c copy -bsf:v trace_headers -f null - 2> NALUS.txt
H264BitStreamParser::H264BitStreamParser()
    : m_nWidth(0),
      m_nHeight(0),
      m_pStart(NULL),
      m_nLength(0),
      m_nCurrentBit(0) {}

H264BitStreamParser::~H264BitStreamParser() {}

unsigned int H264BitStreamParser::ReadBit() {
  assert(m_nCurrentBit <= m_nLength * 8);
  int nIndex = m_nCurrentBit / 8;
  int nOffset = m_nCurrentBit % 8 + 1;

  m_nCurrentBit++;
  return (m_pStart[nIndex] >> (8 - nOffset)) & 0x01;
}
unsigned int H264BitStreamParser::ReadBits(int n) {
  int r = 0;
  int i;
  for (i = 0; i < n; i++) {
    r |= (ReadBit() << (n - i - 1));
  }
  return r;
}
unsigned int H264BitStreamParser::ReadExponentialGolombCode() {
  int r = 0;
  int i = 0;

  while ((ReadBit() == 0) && (i < 32)) {
    i++;
  }

  r = ReadBits(i);
  r += (1 << i) - 1;
  return r;
}
unsigned int H264BitStreamParser::ReadSE() {
  int r = ReadExponentialGolombCode();
  if (r & 0x01) {
    r = (r + 1) / 2;
  } else {
    r = -(r / 2);
  }
  return r;
}
std::string get_frame_type(unsigned char frameType) {
  std::stringstream ss;

  frameType = frameType & 0x1F;
  switch (frameType) {
    case 0:
      // printf("\n printFrameType UNSPECIFIED\n"); fflush(stdout);
      ss << " U";
      break;
    case 1:
      // printf("\n printFrameType Coded slice of a non-IDR picture\n");
      // fflush(stdout);
      ss << " P";
      break;
    case 2:
      ss << "Coded slice data partition A";
      break;
    case 3:
      ss << "Coded slice data partition B";
      break;
    case 4:
      ss << "Coded slice data partition C";
      break;
    case 5:  //	Coded slice of an IDR picture
      // printf("\n printFrameType I_FRAME (IDR) received\n"); fflush(stdout);
      ss << " I";
      break;
    case 6:  //	Supplemental enhancement information (SEI)
      // printf("\n printFrameType Supplemental enhancement information (SEI)
      // received\n"); fflush(stdout);
      ss << " SEI";
      break;
    case 7:  //	Sequence parameter set
      ss << " SPS";
      break;
    case 8:  //	Picture parameter set
      ss << " PPS";
      break;
    case 9:  //	Access unit delimiter
      // printf("\n printFrameType Access unit delimiter received\n");
      ss << " AUD";
      break;
    case 10:  //	End of sequence
      ss << "End of sequence received";
      break;
    case 11:  //	End of stream
      ss << "End of stream received";
      break;
    case 12:  //	Filler data
      ss << "Filler data received";
      break;
    case 13:  //	Sequence parameter set extension
      ss << "Sequence parameter set extension received";
      break;
    case 14:  //	Prefix NAL unit
      ss << "Prefix NAL unit received";
      break;
    case 15:  //	Subset sequence parameter set
      ss << "Subset sequence parameter set received";
      break;
    case 16:  //	Reserved
    case 17:
    case 18:
      ss << "Reserved received";
      break;
    case 19:  //	Coded slice of an auxiliary coded picture without
              // partitioning
      ss << "Coded slice of an auxiliary  received";
      break;
    case 20:  //	Coded slice extension
      ss << "Coded slice extension received";
      break;
    case 21:  //	Coded slice extension for depth view components
      ss << "Coded slice extension received";
      break;
    case 22:  //	Reserved
    case 23:
      ss << "Reserved received";
      break;
      // 1 - 23     NAL unit  Single NAL unit packet             5.6
    case 24:  // STAP - A    Single - time aggregation packet     5.7.1
      ss << "STAP - A received";
      break;
    case 25:  // STAP - B    Single - time aggregation packet     5.7.1
      ss << "STAP - B received";
      break;
    case 26:  // MTAP16    Multi - time aggregation packet      5.7.2
      ss << "MTAP16 received";
      break;
    case 27:  // MTAP24    Multi - time aggregation packet      5.7.2
      ss << "MTAP24 received";
      break;
    case 28:  // FU - A      Fragmentation unit                 5.8
      ss << "FU - A received";
      break;
    case 29:  // FU - B      Fragmentation unit                 5.8
      ss << "FU - B received";
      break;
    case 30:  // reserved
    case 31:  // reserved
    default:
      ss << "Reserved received ** " << frameType;
      break;
  }
  return ss.str();
}
const unsigned char *find_nal(const unsigned char *pStart, unsigned short nLen) {
  const unsigned char *ret = NULL;
  for (size_t i = 0; i < nLen - 4; i++) {
    if ((pStart[i + 0] == 0x00) && (pStart[i + 1] == 0x00) &&
        (pStart[i + 2] == 0x00) && (pStart[i + 3] == 0x01)) {
      ret = (pStart + i);
      break;
    }
  }
  return ret;
}
bool H264BitStreamParser::ParseNAL(const unsigned char *pStart,
                                   unsigned short nLen) {
  //printf("+++++++++++++++++++++++++++++++++++++ len %d \n", nLen);
  int call_id = 0;
  int i = 0;
  if (nLen > 6) {
    // for (size_t i = 0; i < nLen - 4; ) {
    if ((pStart[i + 0] == 0x00) && (pStart[i + 1] == 0x00) &&
        (pStart[i + 2] == 0x00) && (pStart[i + 3] == 0x01)) {
      call_id++;
      // printf("\n NAL[%d]: %d %d %d %d %#02x %s\n", call_id, pStart[i + 0],
      // pStart[i + 1], pStart[i + 2], pStart[i + 3], pStart[i + 4],
      // get_frame_type(pStart[i + 4]).c_str()); printFrameType(pStart[i + 4]);
      // if(ucpInBuffer[i+4] & 0x0F ==0x07)
      if (pStart[i + 4] == 0x67 || pStart[i + 4] == 0x27) {
        // printf("\n ************ SPS: %d %d %d %d %d\n", pStart[i + 0],
        //        pStart[i + 1], pStart[i + 2], pStart[i + 3], pStart[i + 4]);
        if ((m_nWidth > 0) && (m_nHeight > 0)) {
          //printf("\n Not parsing\n");
          return true;
        }
        m_pStart = (unsigned char *)(pStart + i + 5);
        m_nLength = nLen - 5;
        m_nCurrentBit = 0;
        int frame_crop_left_offset = 0;
        int frame_crop_right_offset = 0;
        int frame_crop_top_offset = 0;
        int frame_crop_bottom_offset = 0;

        int profile_idc = ReadBits(8);
        int constraint_set0_flag = ReadBit();
        int constraint_set1_flag = ReadBit();
        int constraint_set2_flag = ReadBit();
        int constraint_set3_flag = ReadBit();
        int constraint_set4_flag = ReadBit();
        int constraint_set5_flag = ReadBit();
        int reserved_zero_2bits = ReadBits(2);
        int level_idc = ReadBits(8);
        int seq_parameter_set_id = ReadExponentialGolombCode();

        if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 ||
            profile_idc == 244 || profile_idc == 44 || profile_idc == 83 ||
            profile_idc == 86 || profile_idc == 118) {
          int chroma_format_idc = ReadExponentialGolombCode();

          if (chroma_format_idc == 3) {
            int residual_colour_transform_flag = ReadBit();
          }
          int bit_depth_luma_minus8 = ReadExponentialGolombCode();
          int bit_depth_chroma_minus8 = ReadExponentialGolombCode();
          int qpprime_y_zero_transform_bypass_flag = ReadBit();
          int seq_scaling_matrix_present_flag = ReadBit();

          if (seq_scaling_matrix_present_flag) {
            int i = 0;
            for (i = 0; i < 8; i++) {
              int seq_scaling_list_present_flag = ReadBit();
              if (seq_scaling_list_present_flag) {
                int sizeOfScalingList = (i < 6) ? 16 : 64;
                int lastScale = 8;
                int nextScale = 8;
                int j = 0;
                for (j = 0; j < sizeOfScalingList; j++) {
                  if (nextScale != 0) {
                    int delta_scale = ReadSE();
                    nextScale = (lastScale + delta_scale + 256) % 256;
                  }
                  lastScale = (nextScale == 0) ? lastScale : nextScale;
                }
              }
            }
          }
        }

        int log2_max_frame_num_minus4 = ReadExponentialGolombCode();
        int pic_order_cnt_type = ReadExponentialGolombCode();
        if (pic_order_cnt_type == 0) {
          int log2_max_pic_order_cnt_lsb_minus4 = ReadExponentialGolombCode();
        } else if (pic_order_cnt_type == 1) {
          int delta_pic_order_always_zero_flag = ReadBit();
          int offset_for_non_ref_pic = ReadSE();
          int offset_for_top_to_bottom_field = ReadSE();
          int num_ref_frames_in_pic_order_cnt_cycle =
              ReadExponentialGolombCode();
          int i;
          for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            ReadSE();
            // sps->offset_for_ref_frame[ i ] = ReadSE();
          }
        }
        int max_num_ref_frames = ReadExponentialGolombCode();
        int gaps_in_frame_num_value_allowed_flag = ReadBit();
        int pic_width_in_mbs_minus1 = ReadExponentialGolombCode();
        int pic_height_in_map_units_minus1 = ReadExponentialGolombCode();
        int frame_mbs_only_flag = ReadBit();
        if (!frame_mbs_only_flag) {
          int mb_adaptive_frame_field_flag = ReadBit();
        }
        int direct_8x8_inference_flag = ReadBit();
        int frame_cropping_flag = ReadBit();
        if (frame_cropping_flag) {
          frame_crop_left_offset = ReadExponentialGolombCode();
          frame_crop_right_offset = ReadExponentialGolombCode();
          frame_crop_top_offset = ReadExponentialGolombCode();
          frame_crop_bottom_offset = ReadExponentialGolombCode();
        }
        int vui_parameters_present_flag = ReadBit();
        pStart++;

        // int Width = ((pic_width_in_mbs_minus1 +1)*16) -
        // frame_crop_bottom_offset*2 - frame_crop_top_offset*2;
        int Width = ((pic_width_in_mbs_minus1 + 1) * 16) -
                    frame_crop_right_offset * 2 - frame_crop_left_offset * 2;
        // int Height = ((2 - frame_mbs_only_flag)*
        // (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_right_offset *
        // 2) - (frame_crop_left_offset * 2);
        int Height = ((2 - frame_mbs_only_flag) *
                      (pic_height_in_map_units_minus1 + 1) * 16) -
                     (frame_crop_bottom_offset * 2) -
                     (frame_crop_top_offset * 2);
        m_nWidth = Width;
        m_nHeight = Height;
        std::cout << "H264BitStreamParser: WxH = " << Width << "x" << Height << std::endl;
      } else if (pStart[i + 4] & 0x1F == 0x06) {
        std::cout << "H264BitStreamParser: SEI found of length = " << nLen << std::endl;
        // printf("\n ************ SEI: %d %d %d %d %d\n", pStart[i + 0],
        //        pStart[i + 1], pStart[i + 2], pStart[i + 3], pStart[i + 4]);
      }
      i = i + 4;
      return true;
    } else {
      i = i + 1;
      std::cout << "H264BitStreamParser: ERROR: Not a valid NAL unit: [header not found]  " << nLen << std::endl;
    }
    //}
  } else {
    std::cout << "H264BitStreamParser: ERROR: Not a valid NAL unit: [short length] " << nLen << std::endl;
  }
  return false;
}