
#include "mss-bus.h"
#include "packet.h"

/** Local machine address. */
mss_addr local_addr;

/** Contains counters for incoming data packets. */
mss_num incoming_count[ MSS_MAX_ADDR + 1 ];

/** Contains counters which are used to mark outcoming data packets. */
mss_num outcoming_count[ MSS_MAX_ADDR + 1 ];

void init_slave (mss_addr addr) {
    local_addr = addr;
    memset( incoming_count, 0, (MSS_MAX_ADDR + 1) * sizeof(mss_num) );
    memset( outcoming_count, 0, (MSS_MAX_ADDR + 1) * sizeof(mss_num) );
}

int mss_slave_send (mss_addr target_addr, const char* data, size_t data_len) {
    /* Prepare for sending... */
    size_t data_sent = 0;
    MssPacket* packet = (MssPacket*) malloc( sizeof(MssPacket) );
    MssPacket* dat_packet = (MssPacket*) malloc( sizeof(MssPacket) );
    
    /* Watch out, MSS_BROADCAST_ADDR produces invalid pointer, thus counter
     * shall never be used in SDN mode... */
    int* packet_count = outcoming_count + target_addr;
    
    dat_packet->dat.packet_type = MSS_DAT;
    dat_packet->dat.src_addr = local_addr;
    dat_packet->dat.dst_addr = target_addr;
    
    /* Keep sending until all data was sent. */
    while( data_send != data_len ) {
        int recv_res = receive_mss_packet( packet, MSS_TIMEOUT );
        
        /* Wait for a bus... */
        if(
            (recv_res == MSS_OK) &&
            (packet->generic.type == MSS_BUS) &&
            (packet->bus.slave_addr == local_addr)
        ) {
            /* Prepare packet... */
            int copy_bytes = data_len - data_sent;
            if( copy_bytes > 10 )
                copy_bytes = 10;
            /* SDN's not counted. */
            if( target_addr != MSS_BROADCAST_ADDR ) {
                ++(*packet_count);
                dat_packet->dat.number = (*packet_count);
            } else
                dat_packet->dat.number = 0;
            dat_packet->dat.data_len = copy_bytes;
            memcpy( dat_packet->dat.data, data + data_sent, copy_bytes );
            CRC_FOR_DAT( dat_packet );
            /* Send packet. */
            send_mss_packet( dat_packet );
            
            /* Wait for a response (if SDA) */
            if( target_addr != MSS_BROADCAST_ADDR ) {
                recv_res = receive_mss_packet( packet, MSS_TIMEOUT );
                
                /* Got ACK */
                if( (recv_res == MSS_OK) && (packet->generic.type == MSS_ACK) )
                    data_sent += copy_bytes;
    
                /* No ACK ;( */
                else {
                    --(*packet_count);
                    /*Note: zakomentowanej tej linii sprawi, iz protokol bedzie
                     *      zapewnial poprawnosc transmisji (retransmitowal da-
                            ne do skutku). */
                    free( packet );
                    free( dat_packet );
                    return data_sent;
                }
            
            /* SDN - no waiting needed, just increase data counter. */
            } else data_sent += copy_bytes;
        }
        
    } /* while has data to send */
    
    free( packet );
    free( dat_packet );
    return data_sent; /* Success - sent all the data. */
}

int mss_slave_recv (mss_addr* sender_addr, const char* buffer, int* is_broadcast) {
    
}