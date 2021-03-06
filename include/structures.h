/*! \file structures.h
    \brief Contains nitty-gritty DVB details.
*/
#ifndef STRUCTURES_H
#define STRUCTURES_H


// C includes
#include <stdint.h>


/// \defgroup structure DVB table retrieval interface
/// \addtogroup structure
/// @{
/// \brief These functions retrieve the corresponding structures from the
/// stream, performing the needed network-to-host conversions.

/// \brief Header that is part of all tables.
struct table_header
{
    uint8_t tid; ///< Table id.

    union
    {
        struct
        {
            uint16_t len : 12; ///< Length of the rest of the table.
            uint16_t res : 2;
            uint16_t zero : 1;
            uint16_t ssi : 1;
        } b1s;

        uint16_t bitfield1;
    } b1u;
} __attribute__((packed));


/// \brief Represents PAT header.
struct pat_header
{
    struct table_header hdr;

    uint16_t tsi;

    struct
    {
        uint8_t cni : 1;
        uint8_t version : 5;
        uint8_t res : 2;
    } b1s;

    uint8_t sec;
    uint8_t lsn;
} __attribute__((packed));


/// \brief Represents PAT body.
struct pat_body
{
    uint16_t ch_num; ///< Channel number of the current PMT.

    union
    {
        struct
        {
            uint16_t pid : 13; ///< PID of the PMT table.
            uint16_t res : 3;
        } b1s;

        uint16_t bitfield1;
    } b1u;
} __attribute__((packed));


/// \brief Represents PMT header.
struct pmt_header
{
    struct table_header hdr;

    uint16_t ch_num; ///< Channel number.

    struct
    {
        uint8_t cni : 1;
        uint8_t version : 5;
        uint8_t res : 2;
    } b;

    uint8_t sec;
    uint8_t lsn;

    union
    {
        struct
        {
            uint16_t pcr_pid : 13;
            uint16_t res2 : 3;
        } b1s;

        uint16_t bitfield1;
    } b1u;

    union
    {
        struct
        {
            uint16_t pilen : 12; ///< Length of the \ref pmt_body section.
            uint16_t res3 : 4;
        } b2s;

        uint16_t bitfield2;
    } b2u;
} __attribute__((packed));


/// \brief Represents PMT body.
struct pmt_body
{
    uint8_t type; ///< Type of stream.

    union
    {
        struct
        {
	    uint16_t pid : 13; ///< PID of the PS.
            uint16_t res : 3;        
        } b1s;

        uint16_t bitfield1;
    } b1u;


    union
    {
        struct
        {
	    uint16_t esilen : 12; ///< Length of the descriptor section.
            uint16_t res2 : 4;        
        } b2s;

        uint16_t bitfield2;
    } b2u;
} __attribute__((packed));


/// \brief Represents the teletext descriptor header.
struct teletext_descriptor_header
{
    uint8_t tag; ///< Tag, should be 0x56.
    uint8_t len; ///< Length of the descriptor.
};


/// \brief Represents SDT header.
struct sdt_header
{
    struct table_header hdr;
	
    uint16_t tsi;
	
    struct
    {
        uint8_t cni : 1;
        uint8_t version : 5;
        uint8_t res : 2;
    } b1s;
    
    uint8_t sec;
    uint8_t lsn;
    
    uint16_t oni;
    uint8_t res2;
} __attribute__((packed));


/// \brief Represents SDT body.
struct sdt_body
{
    uint16_t sid; ///< Service id (same as channel number).
	
    struct
    {
	uint8_t epff : 1;
	uint8_t esf : 1;
	uint8_t res : 6;
    } b1s;
	
    union
    {
	struct
	{
	    uint16_t dlen : 12; ///< Length of the descriptor section.
	    uint16_t fcm : 1;
	    uint16_t rs : 3;
	} b2s;
		
	uint16_t bitfield2;
    } b2u;
} __attribute__((packed));


/// \brief Represents the first half of the service descriptor.
struct sdt_descriptor1
{
    uint8_t tag;
    uint8_t len; ///< Length of the descriptor.
    uint8_t type; ///< Type of service (channel).
    uint8_t spnlen; ///< Length of the service provider name.
} __attribute__((packed));


/// \brief Represents the second half of the service descriptor.
struct sdt_descriptor2
{
    uint8_t snlen; ///< Length of the service name.
} __attribute__((packed));


/// \brief Represents TOT header.
struct tot_header
{
    struct table_header hdr;
	
    uint8_t time[5]; ///< Brain-dead encoded time information.
	
    union
    {
	struct
	{
	    uint16_t dlen : 12; ///< Length of the descriptor section.
	    uint16_t res : 4;
	} b1s;
		
	uint16_t bitfield1;
    } b1u;
} __attribute__((packed));


/// \brief Represents TOT descriptor header.
struct tot_descriptor_header
{
    uint8_t tag;
    uint8_t len; ///< Length of the descriptor body.
} __attribute__((packed));


/// \brief Represents TOT descriptor body.
struct tot_descriptor_body
{
    union
    {
        struct
        {
            uint32_t cc : 24;
            uint32_t regid : 6;
            uint32_t res : 1;
            uint32_t pol : 1;
        } b1s;

        uint32_t bitfield1;
    } b1u;

    uint16_t lto; ///< Local time offset.
    uint8_t toc[5];
    uint16_t nto;
} __attribute__((packed));


/// \brief Retrieves \ref pat_header from the stream.
struct pat_header get_pat_header(const uint8_t *buffer);
/// \brief Retrieves \ref pat_body from the stream.
struct pat_body get_pat_body(const uint8_t *buffer);

/// \brief Retrieves \ref pmt_header from the stream.
struct pmt_header get_pmt_header(const uint8_t *buffer);
/// \brief Retrieves \ref pmt_body from the stream.
struct pmt_body get_pmt_body(const uint8_t *buffer);
/// \brief Retrieves \ref teletext_descriptor_header from the stream.
struct teletext_descriptor_header
    get_teletext_descriptor_header(const uint8_t *buffer);

/// \brief Retrieves \ref sdt_header from the stream.
struct sdt_header get_sdt_header(const uint8_t *buffer);
/// \brief Retrieves \ref sdt_body from the stream.
struct sdt_body get_sdt_body(const uint8_t *buffer);
/// \brief Retrieves \ref sdt_descriptor1 from the stream.
struct sdt_descriptor1 get_sdt_descriptor1(const uint8_t *buffer);
/// \brief Retrieves \ref sdt_descriptor2 from the stream.
struct sdt_descriptor2 get_sdt_descriptor2(const uint8_t *buffer);

/// \brief Retrieves \ref tot_header from the stream.
struct tot_header get_tot_header(const uint8_t *buffer);
/// \brief Retrieves \ref tot_descriptor_header from the stream.
struct tot_descriptor_header get_tot_descriptor_header(const uint8_t *buffer);
/// \brief Retrieves \ref tot_descriptor_body from the stream.
struct tot_descriptor_body get_tot_descriptor_body(const uint8_t *buffer);
/// @}


#endif

