

#ifdef __cplusplus
extern "C"{
#endif

//int vxi_discovery(char* Address, char* idnResponse);
int vxi_discovery(char* szAddress, DWORD* dwPort);
int IsGageSystem(char* szIpAddress);

#ifdef __cplusplus
}
#endif