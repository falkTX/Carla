/*
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __ALSA_SEQ_EVENT_COMPAT_H
#define __ALSA_SEQ_EVENT_COMPAT_H

/**
 * Sequencer event data type
 */
typedef unsigned char snd_seq_event_type_t;

/** Sequencer event type */
enum snd_seq_event_type {
        /** system status; event data type = #snd_seq_result_t */
        SND_SEQ_EVENT_SYSTEM = 0,
        /** returned result status; event data type = #snd_seq_result_t */
        SND_SEQ_EVENT_RESULT,

        /** note on and off with duration; event data type = #snd_seq_ev_note_t */
        SND_SEQ_EVENT_NOTE = 5,
        /** note on; event data type = #snd_seq_ev_note_t */
        SND_SEQ_EVENT_NOTEON,
        /** note off; event data type = #snd_seq_ev_note_t */
        SND_SEQ_EVENT_NOTEOFF,
        /** key pressure change (aftertouch); event data type = #snd_seq_ev_note_t */
        SND_SEQ_EVENT_KEYPRESS,

        /** controller; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_CONTROLLER = 10,
        /** program change; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_PGMCHANGE,
        /** channel pressure; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_CHANPRESS,
        /** pitchwheel; event data type = #snd_seq_ev_ctrl_t; data is from -8192 to 8191) */
        SND_SEQ_EVENT_PITCHBEND,
        /** 14 bit controller value; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_CONTROL14,
        /** 14 bit NRPN;  event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_NONREGPARAM,
        /** 14 bit RPN; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_REGPARAM,

        /** SPP with LSB and MSB values; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_SONGPOS = 20,
        /** Song Select with song ID number; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_SONGSEL,
        /** midi time code quarter frame; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_QFRAME,
        /** SMF Time Signature event; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_TIMESIGN,
        /** SMF Key Signature event; event data type = #snd_seq_ev_ctrl_t */
        SND_SEQ_EVENT_KEYSIGN,

        /** MIDI Real Time Start message; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_START = 30,
        /** MIDI Real Time Continue message; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_CONTINUE,
        /** MIDI Real Time Stop message; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_STOP,
        /** Set tick queue position; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_SETPOS_TICK,
        /** Set real-time queue position; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_SETPOS_TIME,
        /** (SMF) Tempo event; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_TEMPO,
        /** MIDI Real Time Clock message; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_CLOCK,
        /** MIDI Real Time Tick message; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_TICK,
        /** Queue timer skew; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_QUEUE_SKEW,
        /** Sync position changed; event data type = #snd_seq_ev_queue_control_t */
        SND_SEQ_EVENT_SYNC_POS,

        /** Tune request; event data type = none */
        SND_SEQ_EVENT_TUNE_REQUEST = 40,
        /** Reset to power-on state; event data type = none */
        SND_SEQ_EVENT_RESET,
        /** Active sensing event; event data type = none */
        SND_SEQ_EVENT_SENSING,

        /** Echo-back event; event data type = any type */
        SND_SEQ_EVENT_ECHO = 50,
        /** OSS emulation raw event; event data type = any type */
        SND_SEQ_EVENT_OSS,

        /** New client has connected; event data type = #snd_seq_addr_t */
        SND_SEQ_EVENT_CLIENT_START = 60,
        /** Client has left the system; event data type = #snd_seq_addr_t */
        SND_SEQ_EVENT_CLIENT_EXIT,
        /** Client status/info has changed; event data type = #snd_seq_addr_t */
        SND_SEQ_EVENT_CLIENT_CHANGE,
        /** New port was created; event data type = #snd_seq_addr_t */
        SND_SEQ_EVENT_PORT_START,
        /** Port was deleted from system; event data type = #snd_seq_addr_t */
        SND_SEQ_EVENT_PORT_EXIT,
        /** Port status/info has changed; event data type = #snd_seq_addr_t */
        SND_SEQ_EVENT_PORT_CHANGE,

        /** Ports connected; event data type = #snd_seq_connect_t */
        SND_SEQ_EVENT_PORT_SUBSCRIBED,
        /** Ports disconnected; event data type = #snd_seq_connect_t */
        SND_SEQ_EVENT_PORT_UNSUBSCRIBED,

        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR0 = 90,
        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR1,
        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR2,
        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR3,
        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR4,
        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR5,
        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR6,
        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR7,
        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR8,
        /** user-defined event; event data type = any (fixed size) */
        SND_SEQ_EVENT_USR9,

        /** NOP; ignored in any case */
        SND_SEQ_EVENT_NONE = 255
};

/** Sequencer event address */
typedef struct snd_seq_addr {
        unsigned char client;	/**< Client id */
        unsigned char port;	/**< Port id */
} snd_seq_addr_t;

/** Connection (subscription) between ports */
typedef struct snd_seq_connect {
        snd_seq_addr_t sender;	/**< sender address */
        snd_seq_addr_t dest;	/**< destination address */
} snd_seq_connect_t;

/** Real-time data record */
typedef struct snd_seq_real_time {
        unsigned int tv_sec;		/**< seconds */
        unsigned int tv_nsec;		/**< nanoseconds */
} snd_seq_real_time_t;

/** (MIDI) Tick-time data record */
typedef unsigned int snd_seq_tick_time_t;

/** unioned time stamp */
typedef union snd_seq_timestamp {
        snd_seq_tick_time_t tick;       /**< tick-time */
        struct snd_seq_real_time time;  /**< real-time */
} snd_seq_timestamp_t;

/** Note event */
typedef struct snd_seq_ev_note {
        unsigned char channel;		/**< channel number */
        unsigned char note;		/**< note */
        unsigned char velocity;		/**< velocity */
        unsigned char off_velocity;	/**< note-off velocity; only for #SND_SEQ_EVENT_NOTE */
        unsigned int duration;		/**< duration until note-off; only for #SND_SEQ_EVENT_NOTE */
} snd_seq_ev_note_t;

/** Controller event */
typedef struct snd_seq_ev_ctrl {
        unsigned char channel;		/**< channel number */
        unsigned char unused[3];	/**< reserved */
        unsigned int param;		/**< control parameter */
        signed int value;		/**< control value */
} snd_seq_ev_ctrl_t;

/** generic set of bytes (12x8 bit) */
typedef struct snd_seq_ev_raw8 {
        unsigned char d[12];		/**< 8 bit value */
} snd_seq_ev_raw8_t;

/** generic set of integers (3x32 bit) */
typedef struct snd_seq_ev_raw32 {
        unsigned int d[3];		/**< 32 bit value */
} snd_seq_ev_raw32_t;

/** external stored data */
typedef struct snd_seq_ev_ext {
        unsigned int len;		/**< length of data */
        void *ptr;			/**< pointer to data (note: can be 64-bit) */
}
#ifndef _MSC_VER
__attribute__((packed))
#endif
snd_seq_ev_ext_t;

/** Result events */
typedef struct snd_seq_result {
        int event;		/**< processed event type */
        int result;		/**< status */
} snd_seq_result_t;

/** Queue skew values */
typedef struct snd_seq_queue_skew {
        unsigned int value;	/**< skew value */
        unsigned int base;	/**< skew base */
} snd_seq_queue_skew_t;

/** queue timer control */
typedef struct snd_seq_ev_queue_control {
        unsigned char queue;			/**< affected queue */
        unsigned char unused[3];		/**< reserved */
        union {
                signed int value;		/**< affected value (e.g. tempo) */
                snd_seq_timestamp_t time;	/**< time */
                unsigned int position;		/**< sync position */
                snd_seq_queue_skew_t skew;	/**< queue skew */
                unsigned int d32[2];		/**< any data */
                unsigned char d8[8];		/**< any data */
        } param;				/**< data value union */
} snd_seq_ev_queue_control_t;

/** Sequencer event */
typedef struct snd_seq_event {
        snd_seq_event_type_t type;      /**< event type */
        unsigned char flags;            /**< event flags */
        unsigned char tag;              /**< tag */

        unsigned char queue;            /**< schedule queue */
        snd_seq_timestamp_t time;       /**< schedule time */

        snd_seq_addr_t source;          /**< source address */
        snd_seq_addr_t dest;            /**< destination address */

        union {
                snd_seq_ev_note_t note;         /**< note information */
                snd_seq_ev_ctrl_t control;      /**< MIDI control information */
                snd_seq_ev_raw8_t raw8;         /**< raw8 data */
                snd_seq_ev_raw32_t raw32;       /**< raw32 data */
                snd_seq_ev_ext_t ext;           /**< external data */
                snd_seq_ev_queue_control_t queue; /**< queue control */
                snd_seq_timestamp_t time;       /**< timestamp */
                snd_seq_addr_t addr;            /**< address */
                snd_seq_connect_t connect;      /**< connect information */
                snd_seq_result_t result;        /**< operation result code */
        } data;                         /**< event data... */
} snd_seq_event_t;

#endif /* __ALSA_SEQ_EVENT_COMPAT_H */

