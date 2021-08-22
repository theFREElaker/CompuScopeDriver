typedef struct {
  PCI_CAPABILITIES_HEADER         Header;
  struct _PCI_MSI_MESSAGE_CONTROL {
    USHORT MSIEnable  :1;
    USHORT MultipleMessageCapable  :3;
    USHORT MultipleMessageEnable  :3;
    USHORT CapableOf64Bits  :1;
    USHORT PerVectorMaskCapable  :1;
    USHORT Reserved  :7;
  } MessageControl;
  union {
    struct _PCI_MSI_MESSAGE_ADDRESS {
      ULONG Reserved  :2;
      ULONG Address  :30;
    } Register;
    ULONG                           Raw;
  } MessageAddressLower;
  union {
    struct {
      USHORT MessageData;
    } Option32Bit;
    struct {
      ULONG  MessageAddressUpper;
      USHORT MessageData;
      USHORT Reserved;
      ULONG  MaskBits;
      ULONG  PendingBits;
    } Option64Bit;
  };
} PCI_MSI_CAPABILITY, *PPCI_MSI_CAPABILITY;

typedef struct
{
  union {
    struct {
      ULONG BaseIndexRegister  :3;
      ULONG Reserved  :29;
    };
    ULONG  TableOffset;
  };
} PCIX_TABLE_POINTER, *PPCIX_TABLE_POINTER;

typedef struct
{
  PCI_CAPABILITIES_HEADER Header;
  struct {
    USHORT TableSize  :11;
    USHORT Reserved  :3;
    USHORT FunctionMask  :1;
    USHORT MSIXEnable  :1;
  } MessageControl;
  PCIX_TABLE_POINTER      MessageTable;
  PCIX_TABLE_POINTER      PBATable;
} PCI_MSIX_CAPABILITY, *PPCI_MSIX_CAPABILITY;
