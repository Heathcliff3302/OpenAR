#include "v4l2.h"
#include <sys/stat.h>
#include <dirent.h>

#define OUTPUT_DIR "capture_dir"

struct buffer {
    void *start;
    size_t length;
};

static std::vector<buffer> buffers;
static int fd = -1;
static struct v4l2_requestbuffers g_req;
static enum v4l2_buf_type g_type;

// YUYV to RGB24 轉換函數
static void YUY2ToRGB24(unsigned char *yuy2, int src_stride, 
                        unsigned char *rgb24, int dst_stride, 
                        int width, int height) {
    for (int y = 0; y < height; y++) {
        unsigned char *src = yuy2 + y * src_stride;
        unsigned char *dst = rgb24 + y * dst_stride;
        
        for (int x = 0; x < width; x += 2) {
            int Y0 = src[0];
            int U  = src[1];
            int Y1 = src[2];
            int V  = src[3];
            
            // 轉換第一個像素
            int C = Y0 - 16;
            int D = U - 128;
            int E = V - 128;
            
            int R0 = (298 * C + 409 * E + 128) >> 8;
            int G0 = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int B0 = (298 * C + 516 * D + 128) >> 8;
            
            dst[0] = (unsigned char)(R0 < 0 ? 0 : (R0 > 255 ? 255 : R0));
            dst[1] = (unsigned char)(G0 < 0 ? 0 : (G0 > 255 ? 255 : G0));
            dst[2] = (unsigned char)(B0 < 0 ? 0 : (B0 > 255 ? 255 : B0));
            
            // 轉換第二個像素
            C = Y1 - 16;
            int R1 = (298 * C + 409 * E + 128) >> 8;
            int G1 = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int B1 = (298 * C + 516 * D + 128) >> 8;
            
            dst[3] = (unsigned char)(R1 < 0 ? 0 : (R1 > 255 ? 255 : R1));
            dst[4] = (unsigned char)(G1 < 0 ? 0 : (G1 > 255 ? 255 : G1));
            dst[5] = (unsigned char)(B1 < 0 ? 0 : (B1 > 255 ? 255 : B1));
            
            src += 4;
            dst += 6;
        }
    }
}

static void yuyv_to_rgb(unsigned char *yuyv, unsigned char *rgb, int width, int height)
{
    const int src_stride = width * 2;  // YUYV 每像素 2 字節
    const int dst_stride = width * 3;  // RGB24 每像素 3 字節
    YUY2ToRGB24(yuyv, src_stride, rgb, dst_stride, width, height);
}

// JPEG壓縮到內存
struct jpeg_mem_dest_mgr {
    struct jpeg_destination_mgr pub;
    std::vector<unsigned char> *buffer;
};

static void jpeg_mem_init_destination(j_compress_ptr cinfo) {
    jpeg_mem_dest_mgr *dest = (jpeg_mem_dest_mgr *)cinfo->dest;
    dest->buffer->clear();
    dest->buffer->resize(4096);
    dest->pub.next_output_byte = dest->buffer->data();
    dest->pub.free_in_buffer = dest->buffer->size();
}

static boolean jpeg_mem_empty_output_buffer(j_compress_ptr cinfo) {
    jpeg_mem_dest_mgr *dest = (jpeg_mem_dest_mgr *)cinfo->dest;
    size_t old_size = dest->buffer->size();
    dest->buffer->resize(old_size * 2);
    dest->pub.next_output_byte = dest->buffer->data() + old_size;
    dest->pub.free_in_buffer = dest->buffer->size() - old_size;
    return TRUE;
}

static void jpeg_mem_term_destination(j_compress_ptr cinfo) {
    jpeg_mem_dest_mgr *dest = (jpeg_mem_dest_mgr *)cinfo->dest;
    size_t data_size = dest->buffer->size() - dest->pub.free_in_buffer;
    dest->buffer->resize(data_size);
}

static void jpeg_mem_dest(j_compress_ptr cinfo, std::vector<unsigned char> *buffer) {
    jpeg_mem_dest_mgr *dest;
    if (cinfo->dest == nullptr) {
        cinfo->dest = (struct jpeg_destination_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT,
                                       sizeof(jpeg_mem_dest_mgr));
    }
    dest = (jpeg_mem_dest_mgr *)cinfo->dest;
    dest->pub.init_destination = jpeg_mem_init_destination;
    dest->pub.empty_output_buffer = jpeg_mem_empty_output_buffer;
    dest->pub.term_destination = jpeg_mem_term_destination;
    dest->buffer = buffer;
}

// 將RGB數據壓縮為JPEG並轉換為base64
static std::string rgb_to_base64(unsigned char *rgb, int width, int height, int quality) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    std::vector<unsigned char> jpeg_buffer;
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &jpeg_buffer);
    
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    
    int row_stride = width * 3;
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &rgb[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    
    // 將JPEG二進制數據轉換為base64
    static const char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    for (size_t idx = 0; idx < jpeg_buffer.size(); idx++) {
        char_array_3[i++] = jpeg_buffer[idx];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = 0;
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
            ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
            ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        
        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];
        
        while (i++ < 3)
            ret += '=';
    }
    
    return ret;
}

// 獲取當前時間戳（毫秒）
long long now_ms() {
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return ms;
}

// 創建輸出目錄
int create_output_dir() {
    struct stat st;
    if (stat(OUTPUT_DIR, &st) != 0) {
        if (mkdir(OUTPUT_DIR, 0755) != 0) {
            perror("mkdir");
            return -1;
        }
    }
    return 0;
}

int v4l2_init(const char *device)
{
    struct v4l2_format fmt;
    struct v4l2_buffer buf;

    /* 1. 打开设备 */
    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    /* 2. 设置格式 */
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT");
        return -1;
    }

    /* 3. 申请 buffer */
    memset(&g_req, 0, sizeof(g_req));
    g_req.count  = BUFFER_COUNT;
    g_req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    g_req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &g_req) < 0) {
        perror("VIDIOC_REQBUFS");
        return -1;
    }

    buffers.assign(g_req.count, {});

    /* 4. mmap + QBUF */
    for (unsigned int i = 0; i < g_req.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("VIDIOC_QUERYBUF");
            return -1;
        }

        buffers[i].length = buf.length;
        buffers[i].start  = mmap(nullptr, buf.length,
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED,
                                 fd, buf.m.offset);

        if (buffers[i].start == MAP_FAILED) {
            perror("mmap");
            return -1;
        }

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            return -1;
        }
    }

    return 0;
}

std::vector<std::string> v4l2_work()
{
    struct v4l2_buffer buf;
    std::vector<std::string> base64_images;

    /* 5. 开始采集 */
    g_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &g_type) < 0) {
        perror("VIDIOC_STREAMON");
        return base64_images;  // 返回空vector
    }

    std::unique_ptr<unsigned char[]> rgb_buffer(
        new unsigned char[WIDTH * HEIGHT * 3]
    );

    long long last_time = 0;
    long long start_time = now_ms();
    const long long MAX_CAPTURE_TIME_MS = 10000;  // 最多采集10秒

    while (1) {
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
            perror("VIDIOC_DQBUF");
            break;
        }

        long long now = now_ms();

        // 檢查是否超過最大采集時間
        if (now - start_time >= MAX_CAPTURE_TIME_MS) {
            // 歸還buffer後退出
            ioctl(fd, VIDIOC_QBUF, &buf);
            break;
        }

        // 每隔CAPTURE_INTERVAL_MS采集一幀
        if (last_time == 0 || now - last_time >= CAPTURE_INTERVAL_MS) {
            unsigned char *yuyv =
                (unsigned char *)buffers[buf.index].start;

            yuyv_to_rgb(yuyv, rgb_buffer.get(), WIDTH, HEIGHT);

            // 轉換為base64
            std::string base64_str = rgb_to_base64(rgb_buffer.get(), WIDTH, HEIGHT, 85);
            base64_images.push_back(base64_str);

            last_time = now;
        }

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            break;
        }
    }

    ioctl(fd, VIDIOC_STREAMOFF, &g_type);
    return base64_images;
}

int v4l2_free()
{
    for (unsigned int i = 0; i < g_req.count; i++) {
        if (buffers[i].start && buffers[i].start != MAP_FAILED) {
            munmap(buffers[i].start, buffers[i].length);
        }
    }

    buffers.clear();

    if (fd >= 0) {
        close(fd);
        fd = -1;
    }

    return 0;
}
