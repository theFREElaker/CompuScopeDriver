#include <rpc/types.h>

typedef long Device_Link;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_Link(...);
}
#else
bool_t xdr_Device_Link();
#endif


enum Device_AddrFamily {
	DEVICE_TCP = 0,
	DEVICE_UDP = 1,
};
typedef enum Device_AddrFamily Device_AddrFamily;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_AddrFamily(...);
}
#else
bool_t xdr_Device_AddrFamily();
#endif


typedef long Device_Flags;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_Flags(...);
}
#else
bool_t xdr_Device_Flags();
#endif


typedef long Device_ErrorCode;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_ErrorCode(...);
}
#else
bool_t xdr_Device_ErrorCode();
#endif


struct Device_Error {
	Device_ErrorCode error;
};
typedef struct Device_Error Device_Error;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_Error(...);
}
#else
bool_t xdr_Device_Error();
#endif


struct Create_LinkParms {
	long clientId;
	bool_t lockDevice;
	u_long lock_timeout;
	char *device;
};
typedef struct Create_LinkParms Create_LinkParms;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Create_LinkParms(...);
}
#else
bool_t xdr_Create_LinkParms();
#endif


struct Create_LinkResp {
	Device_ErrorCode error;
	Device_Link lid;
	u_short abortPort;
	u_long maxRecvSize;
};
typedef struct Create_LinkResp Create_LinkResp;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Create_LinkResp(...);
}
#else
bool_t xdr_Create_LinkResp();
#endif


struct Device_WriteParms {
	Device_Link lid;
	u_long io_timeout;
	u_long lock_timeout;
	Device_Flags flags;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct Device_WriteParms Device_WriteParms;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_WriteParms(...);
}
#else
bool_t xdr_Device_WriteParms();
#endif


struct Device_WriteResp {
	Device_ErrorCode error;
	u_long size;
};
typedef struct Device_WriteResp Device_WriteResp;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_WriteResp(...);
}
#else
bool_t xdr_Device_WriteResp();
#endif


struct Device_ReadParms {
	Device_Link lid;
	u_long requestSize;
	u_long io_timeout;
	u_long lock_timeout;
	Device_Flags flags;
	char termChar;
};
typedef struct Device_ReadParms Device_ReadParms;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_ReadParms(...);
}
#else
bool_t xdr_Device_ReadParms();
#endif


struct Device_ReadResp {
	Device_ErrorCode error;
	long reason;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct Device_ReadResp Device_ReadResp;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_ReadResp(...);
}
#else
bool_t xdr_Device_ReadResp();
#endif


struct Device_ReadStbResp {
	Device_ErrorCode error;
	u_char stb;
};
typedef struct Device_ReadStbResp Device_ReadStbResp;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_ReadStbResp(...);
}
#else
bool_t xdr_Device_ReadStbResp();
#endif


struct Device_GenericParms {
	Device_Link lid;
	Device_Flags flags;
	u_long lock_timeout;
	u_long io_timeout;
};
typedef struct Device_GenericParms Device_GenericParms;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_GenericParms(...);
}
#else
bool_t xdr_Device_GenericParms();
#endif


struct Device_RemoteFunc {
	u_long hostAddr;
	u_short hostPort;
	u_long progNum;
	u_long progVers;
	Device_AddrFamily progFamily;
};
typedef struct Device_RemoteFunc Device_RemoteFunc;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_RemoteFunc(...);
}
#else
bool_t xdr_Device_RemoteFunc();
#endif


struct Device_EnableSrqParms {
	Device_Link lid;
	bool_t enable;
	struct {
		u_int handle_len;
		char *handle_val;
	} handle;
};
typedef struct Device_EnableSrqParms Device_EnableSrqParms;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_EnableSrqParms(...);
}
#else
bool_t xdr_Device_EnableSrqParms();
#endif


struct Device_LockParms {
	Device_Link lid;
	Device_Flags flags;
	u_long lock_timeout;
};
typedef struct Device_LockParms Device_LockParms;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_LockParms(...);
}
#else
bool_t xdr_Device_LockParms();
#endif


struct Device_DocmdParms {
	Device_Link lid;
	Device_Flags flags;
	u_long io_timeout;
	u_long lock_timeout;
	long cmd;
	bool_t network_order;
	long datasize;
	struct {
		u_int data_in_len;
		char *data_in_val;
	} data_in;
};
typedef struct Device_DocmdParms Device_DocmdParms;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_DocmdParms(...);
}
#else
bool_t xdr_Device_DocmdParms();
#endif


struct Device_DocmdResp {
	Device_ErrorCode error;
	struct {
		u_int data_out_len;
		char *data_out_val;
	} data_out;
};
typedef struct Device_DocmdResp Device_DocmdResp;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_DocmdResp(...);
}
#else
bool_t xdr_Device_DocmdResp();
#endif


#define DEVICE_ASYNC ((u_long)0x0607B0)
#define DEVICE_ASYNC_VERSION ((u_long)1)
#define device_abort ((u_long)1)
#ifdef __cplusplus
extern "C" {
extern Device_Error *device_abort_1(...);
}
#else
extern Device_Error *device_abort_1();
#endif /* __cplusplus */


#define DEVICE_CORE ((u_long)0x0607AF)
#define DEVICE_CORE_VERSION ((u_long)1)
#define create_link ((u_long)10)
#ifdef __cplusplus
extern "C" {
extern Create_LinkResp *create_link_1(...);
}
#else
extern Create_LinkResp *create_link_1();
#endif /* __cplusplus */
#define device_write ((u_long)11)
#ifdef __cplusplus
extern "C" {
extern Device_WriteResp *device_write_1(...);
}
#else
extern Device_WriteResp *device_write_1();
#endif /* __cplusplus */
#define device_read ((u_long)12)
#ifdef __cplusplus
extern "C" {
extern Device_ReadResp *device_read_1(...);
}
#else
extern Device_ReadResp *device_read_1();
#endif /* __cplusplus */
#define device_readstb ((u_long)13)
#ifdef __cplusplus
extern "C" {
extern Device_ReadStbResp *device_readstb_1(...);
}
#else
extern Device_ReadStbResp *device_readstb_1();
#endif /* __cplusplus */
#define device_trigger ((u_long)14)
#ifdef __cplusplus
extern "C" {
extern Device_Error *device_trigger_1(...);
}
#else
extern Device_Error *device_trigger_1();
#endif /* __cplusplus */
#define device_clear ((u_long)15)
#ifdef __cplusplus
extern "C" {
extern Device_Error *device_clear_1(...);
}
#else
extern Device_Error *device_clear_1();
#endif /* __cplusplus */
#define device_remote ((u_long)16)
#ifdef __cplusplus
extern "C" {
extern Device_Error *device_remote_1(...);
}
#else
extern Device_Error *device_remote_1();
#endif /* __cplusplus */
#define device_local ((u_long)17)
#ifdef __cplusplus
extern "C" {
extern Device_Error *device_local_1(...);
}
#else
extern Device_Error *device_local_1();
#endif /* __cplusplus */
#define device_lock ((u_long)18)
#ifdef __cplusplus
extern "C" {
extern Device_Error *device_lock_1(...);
}
#else
extern Device_Error *device_lock_1();
#endif /* __cplusplus */
#define device_unlock ((u_long)19)
#ifdef __cplusplus
extern "C" {
extern Device_Error *device_unlock_1(...);
}
#else
extern Device_Error *device_unlock_1();
#endif /* __cplusplus */
#define device_enable_srq ((u_long)20)
#ifdef __cplusplus
extern "C" {
extern Device_Error *device_enable_srq_1(...);
}
#else
extern Device_Error *device_enable_srq_1();
#endif /* __cplusplus */
#define device_docmd ((u_long)22)
#ifdef __cplusplus
extern "C" {
extern Device_DocmdResp *device_docmd_1(...);
}
#else
extern Device_DocmdResp *device_docmd_1();
#endif /* __cplusplus */
#define destroy_link ((u_long)23)
#ifdef __cplusplus
extern "C" {
extern Device_Error *destroy_link_1(...);
}
#else
extern Device_Error *destroy_link_1();
#endif /* __cplusplus */
#define create_intr_chan ((u_long)25)
#ifdef __cplusplus
extern "C" {
extern Device_Error *create_intr_chan_1(...);
}
#else
extern Device_Error *create_intr_chan_1();
#endif /* __cplusplus */
#define destroy_intr_chan ((u_long)26)
#ifdef __cplusplus
extern "C" {
extern Device_Error *destroy_intr_chan_1(...);
}
#else
extern Device_Error *destroy_intr_chan_1();
#endif /* __cplusplus */


struct Device_SrqParms {
	struct {
		u_int handle_len;
		char *handle_val;
	} handle;
};
typedef struct Device_SrqParms Device_SrqParms;
#ifdef __cplusplus
extern "C" {
bool_t xdr_Device_SrqParms(...);
}
#else
bool_t xdr_Device_SrqParms();
#endif


#define DEVICE_INTR ((u_long)0x0607B1)
#define DEVICE_INTR_VERSION ((u_long)1)
#define device_intr_srq ((u_long)30)
#ifdef __cplusplus
extern "C" {
extern void *device_intr_srq_1(...);
}
#else
extern void *device_intr_srq_1();
#endif /* __cplusplus */

