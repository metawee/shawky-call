/*
 * Shawky Call: pjproject 2.15.1 build configuration
 * (copied to pjlib/include/pj/config_site.h by build/build-deps.ps1).
 *
 * Windows desktop x64, audio only. Video is off (MicroSIP _GLOBAL_VIDEO is
 * disabled in microsip/const.h), so SDL/FFmpeg/libvpx are not needed.
 */
#pragma once

/* TLS + SRTP (incl. DTLS-SRTP, used by MicroSIP's _GLOBAL_DTLS) via OpenSSL */
#define PJ_HAS_SSL_SOCK                 1
#define PJ_SSL_SOCK_IMP                 PJ_SSL_SOCK_IMP_OPENSSL
#define PJMEDIA_HAS_SRTP                1
#define PJMEDIA_SRTP_HAS_DTLS           1

/* No video */
#define PJMEDIA_HAS_VIDEO               0

/* Codecs: G.711 (built-in), G.722, GSM, iLBC, Speex, G.722.1, Opus, G.729 (bcg729) */
#define PJMEDIA_HAS_OPUS_CODEC          1
#define PJMEDIA_HAS_BCG729              1
#define PJMEDIA_HAS_G7221_CODEC         1

/* Windows audio: WMME (pjproject default on desktop Windows) */
#define PJMEDIA_AUDIO_DEV_HAS_WMME      1

/* Room for the main account plus local account, and a few parallel calls */
#define PJSUA_MAX_ACC                   8
#define PJSUA_MAX_CALLS                 8
