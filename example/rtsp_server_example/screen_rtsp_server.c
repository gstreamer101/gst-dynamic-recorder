#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

int main(int argc, char *argv[])
{
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    gst_init(&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE); // GStreamer loop 생성.
    server = gst_rtsp_server_new(); // RTSP 서버 생성.
    gst_rtsp_server_set_service(server, "8554"); // RTSP 포트 설정.
    mounts = gst_rtsp_server_get_mount_points(server); // RTSP URL 경로 지정용 마운트.
    factory = gst_rtsp_media_factory_new(); // 미디어 스트림을 생산하는 팩토리.

    // launch string: 현재 화면을 캡처해서 H264로 인코딩하고 RTP로 송출.
    gst_rtsp_media_factory_set_launch(factory,
        "( ximagesrc use-damage=0 show-pointer=true !"
        "videoconvert !"
        "video/x-raw,format=I420,framerate=60/1 !"
        "x264enc bitrate=15000 speed-preset=veryfast tune=zerolatency key-int-max=60 !"
        "rtph264pay config-interval=1 name=pay0 pt=96 )");

    gst_rtsp_media_factory_set_shared(factory, TRUE); // factory에서 하나의 스트림을 다수의 클라이언트에게 공유.

    // rtsp://0.0.0.0:8554/screen 으로 factory를 연결.
    gst_rtsp_mount_points_add_factory(mounts, "/screen", factory);
    g_object_unref(mounts);

    gst_rtsp_server_attach(server, NULL); // RTSP 서버를 메인 루프에 연결.

    g_print("RTSP server running: rtsp://0.0.0.0:8554/screen\n");
    g_main_loop_run(loop); // GSteamer 루프 시작

    return 0;
}

/*

[파이프라인 구성] (gst_rtsp_media_factory_set_launch())

 ┌────────────┐
 │ ximagesrc  │  ← 현재 화면 캡처 (X11)
 └────┬───────┘
      │
      ▼
┌──────────────┐
│ videoconvert │  ← 호환되는 색상 포맷으로 변환 (예: RGB → YUV)
└────┬─────────┘
     │
     ▼
┌──────────────────────────────┐
│ video/x-raw,format=I420      │  ← YUV 4:2:0으로 강제 지정
│ framerate=60/1               │  ← 초당 60프레임
└────────┬─────────────────────┘
         │
         ▼
 ┌──────────────────────────────────────────────────────────────┐
 │ x264enc                                                      │
 │   - bitrate=15000 (낮으면 화질 저하, 높으면 CPU 부하)             │
 │   - speed-preset=veryfast (인코딩 속도)                        │
 │   - tune=zerolatency (버퍼 최소화하여 지연 줄임. 실시간 스트리밍용    │
 │   - key-int-max=60 I-Frame 삽입 주기 (60프레임마다 1프레임)       │
 │   → H.264 인코딩                                              │
 └────────┬────────────────────────────────────────────────────┘
          │
          ▼
 ┌────────────────────────────────────────────────┐
 │ rtph264pay                                     │
 │   - config-interval=1 (SPS/PPS 정보를 1초마다 삽입)│
 │   - name=pay0 (RTSP 서버 이름 설정)               │
 │   - pt=96 (RTP dynamic payload type)           │
 │   → RTP 패킷화하여 RTSP 서버에 넘김                │
 └────────────────────────────────────────────────┘

 */