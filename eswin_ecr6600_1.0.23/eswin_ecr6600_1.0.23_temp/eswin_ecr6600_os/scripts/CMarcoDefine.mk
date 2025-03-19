
.PHONY:marco

define MacroDefine
	ifeq ($(CONFIG_$(strip $(1))),y)
		CMARCODEFINES+=$(foreach marco, $(2), -D$(marco))
	endif
endef

CMARCODEFINES：=

##################################################################

#Ble Macro

##################################################################

ifeq ($(CONFIG_ECR_BLE),y)
    $(eval $(call MacroDefine,			BLE_GAIA,				CFG_GAIA))
    
    $(eval $(call MacroDefine,			BLE_BUBBLE,				CFG_BUBBLE))
    
    
    ifeq ($(CONFIG_BT_EMB_PRESENT),y)
    
    	CMARCODEFINES+=-DCFG_EMB -DCFG_BT 
    
    	CMARCODEFINES+=-DCFG_DBG_MEM -DCFG_DBG_FLASH -DCFG_DBG_NVDS -DCFG_DBG_STACK_PROF
    
        $(eval $(call MacroDefine,		BLE_RF_RIPPLE,			CFG_RF_RIPPLE))
        $(eval $(call MacroDefine,		BLE_RF_ATLAS,			CFG_RF_ATLAS))
        $(eval $(call MacroDefine,		BLE_RF_EXTRC,			CFG_RF_EXTRC))
    
    
    	
        $(eval $(call MacroDefine,		BLE_RD_PICONET_CLK,		CFG_BT_READ_PICONET_CLOCK))
    	
    
        $(eval $(call MacroDefine,		BLE_WLCOEX,				CFG_WLAN_COEX))	
        $(eval $(call MacroDefine,		BLE_WLCOEXTST,			CFG_WLAN_COEX_TEST))
    	
    	ifeq ($(CONFIG_BLE_SEC_CON),16)
    		CMARCODEFINES+=-DCFG_ECC_16_BITS_ALGO
    	endif
    	ifeq ($(CONFIG_BLE_CRYPTO_UT),y)
            $(eval $(call MacroDefine,	ECR_BLE_DBG,		CFG_CRYPTO_UT))
    	endif
	
		ifeq ($(CONFIG_TRC_ENABLE),y)
                        $(eval $(call MacroDefine,			TRC_ARB,					CFG_TRC_ARB))
                        $(eval $(call MacroDefine,			TRC_PROG,					CFG_TRC_PROG))
                        $(eval $(call MacroDefine,			TRC_SLEEP,					CFG_TRC_SLEEP))
                        $(eval $(call MacroDefine,			TRC_CS_BT,					CFG_TRC_CS_BT))
                        $(eval $(call MacroDefine,			TRC_LMP,					CFG_TRC_LMP))
                        $(eval $(call MacroDefine,			TRC_LC_STATE_TRANS,			CFG_TRC_LC_STATE_TRANS))
		endif #CONFIG_TRC_ENABLE
    	
    	
    endif #CONFIG_BT_EMB_PRESENT
    
    ifeq ($(CONFIG_BLE_EMB_PRESENT),y)
    	CMARCODEFINES+=-DCFG_EMB -DCFG_BLE
    	CMARCODEFINES+=-DCFG_DBG_MEM -DCFG_DBG_FLASH -DCFG_DBG_NVDS -DCFG_DBG_STACK_PROF
        $(eval $(call MacroDefine,		BLE_RF_RIPPLE,			CFG_RF_RIPPLE))
        $(eval $(call MacroDefine,		BLE_RF_ATLAS,			CFG_RF_ATLAS))
        $(eval $(call MacroDefine,		BLE_RF_EXTRC,			CFG_RF_EXTRC))
        $(eval $(call MacroDefine,		BLE_ROLE_CENTRAL,		CFG_CENTRAL))
        $(eval $(call MacroDefine,		BLE_ROLE_OBSERVER,		CFG_OBSERVER))
        $(eval $(call MacroDefine,		BLE_ROLE_BROADCASTER,	CFG_BROADCASTER))
        $(eval $(call MacroDefine,		BLE_ROLE_PERIPHERAL,	CFG_PERIPHERAL))
        $(eval $(call MacroDefine,		BLE_ROLE_PERIPHERAL_OBSERVER,	CFG_PERIPHERAL 	CFG_OBSERVER))
        $(eval $(call MacroDefine,		BLE_ROLE_NONCON,		CFG_BROADCASTER 	CFG_OBSERVER ))
        $(eval $(call MacroDefine,		BLE_ROLE_ALL,			CFG_ALLROLES))
    
    	
    	CMARCODEFINES+=-DCFG_ACT=$(CONFIG_LE_ACT)
    	CMARCODEFINES+=-DCFG_RAL=$(CONFIG_BLE_RAL)
    	
        $(eval $(call MacroDefine,		BLE_AFS_EXT,			CFG_AFS_EXT))
        $(eval $(call MacroDefine,		BLE_HOST_PRESENT,		CFG_CON=$(CONFIG_LE_CONN)))
    	
        $(eval $(call MacroDefine,		BLE_CON_CTE_REQ,		CFG_CON_CTE_REQ))
        $(eval $(call MacroDefine,		BLE_CON_CTE_RSP,		CFG_CON_CTE_RSP))
    	
        $(eval $(call MacroDefine,		BLE_CONLESS_CTE_TX,		CFG_CONLESS_CTE_TX))
        $(eval $(call MacroDefine,		BLE_CONLESS_CTE_RX,		CFG_CONLESS_CTE_RX))
    	
        $(eval $(call MacroDefine,		BLE_AOD,				CFG_AOD))
        $(eval $(call MacroDefine,		BLE_AOA,				CFG_AOA))
    	
    	
        $(eval $(call MacroDefine,		BLE_WLCOEX,				CFG_WLAN_COEX))		
        $(eval $(call MacroDefine,		BLE_WLCOEXTST,			CFG_WLAN_COEX_TEST))
    	
    	ifeq ($(CONFIG_BLE_SEC_CON),16)
    		CMARCODEFINES+=-DCFG_ECC_16_BITS_ALGO
    	endif
	
		ifeq ($(CONFIG_TRC_ENABLE),y)
                        $(eval $(call MacroDefine,		TRC_ARB,		CFG_TRC_ARB))
                        $(eval $(call MacroDefine,		TRC_PROG,		CFG_TRC_PROG))
                        $(eval $(call MacroDefine,		TRC_SLEEP,		CFG_TRC_SLEEP))
                        $(eval $(call MacroDefine,		TRC_CS_BLE,		CFG_TRC_CS_BLE))
                        $(eval $(call MacroDefine,		TRC_LLCP,		CFG_TRC_LLCP))
                        $(eval $(call MacroDefine,		TRC_LLC_STATE_TRANS,		CFG_TRC_LLC_STATE_TRANS))
                        $(eval $(call MacroDefine,		TRC_RX_DESC,		CFG_TRC_RX_DESC))
                        $(eval $(call MacroDefine,		TRC_ADV,		CFG_TRC_ADV))
                        $(eval $(call MacroDefine,		TRC_ACL,		CFG_TRC_ACL))
		endif #CONFIG_TRC_ENABLE

    endif #CONFIG_BLE_EMB_PRESENT
    
    
    
    ifeq ($(CONFIG_BLE_HOST_PRESENT),y)
    
    	CMARCODEFINES+=-DCFG_HOST -DCFG_BLE
    
        $(eval $(call MacroDefine,		BLE_AHITL,				CFG_AHITL))
    		
        $(eval $(call MacroDefine,		BLE_ROLE_CENTRAL,		CFG_CENTRAL))
        $(eval $(call MacroDefine,		BLE_ROLE_OBSERVER,		CFG_OBSERVER))
        $(eval $(call MacroDefine,		BLE_ROLE_BROADCASTER,	CFG_BROADCASTER))
        $(eval $(call MacroDefine,		BLE_ROLE_PERIPHERAL,	CFG_PERIPHERAL))
        $(eval $(call MacroDefine,		BLE_ROLE_NONCON,		CFG_BROADCASTER 	CFG_OBSERVER ))
        $(eval $(call MacroDefine,		BLE_ROLE_ALL,			CFG_ALLROLES))
    	
    	CMARCODEFINES+=-DCFG_ACT=$(CONFIG_LE_ACT)
    	CMARCODEFINES+=-DCFG_RAL=$(CONFIG_BLE_RAL)
    	
        $(eval $(call MacroDefine,		BLE_AFS_EXT,			CFG_AFS_EXT))
    	
    	CMARCODEFINES+=-DCFG_CON=$(CONFIG_LE_CONN)
    
        $(eval $(call MacroDefine,		BLE_EXTDB,				CFG_EXT_DB))
    	
        $(eval $(call MacroDefine,		BLE_ATTC,				CFG_ATTC))
        $(eval $(call MacroDefine,		BLE_ATTS,				CFG_ATTS))
    
    	ifeq ($(CONFIG_BLE_PROFILES),y)
    		CMARCODEFINES+=-DCFG_PRF -DCFG_NB_PRF=$(CONFIG_BLE_NBPRF)
            $(eval $(call MacroDefine,	BLE_PROFILE_DISC,		CFG_PRF_DISC))
            $(eval $(call MacroDefine,	BLE_PROFILE_DISS,		CFG_PRF_DISS))
            $(eval $(call MacroDefine,	BLE_PROFILE_PXPM,		CFG_PRF_PXPM))
            $(eval $(call MacroDefine,	BLE_PROFILE_PXPR,		CFG_PRF_PXPR))
            $(eval $(call MacroDefine,	BLE_PROFILE_FMPL,		CFG_PRF_FMPL))
            $(eval $(call MacroDefine,	BLE_PROFILE_FMPT,		CFG_PRF_FMPT))
            $(eval $(call MacroDefine,	BLE_PROFILE_HTPC,		CFG_PRF_HTPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_HTPT,		CFG_PRF_HTPT))
            $(eval $(call MacroDefine,	BLE_PROFILE_BLPC,		CFG_PRF_BLPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_BLPS,		CFG_PRF_BLPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_HRPC,		CFG_PRF_HRPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_HRPS,		CFG_PRF_HRPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_TIPC,		CFG_PRF_TIP))
            $(eval $(call MacroDefine,	BLE_PROFILE_TIPS,		CFG_PRF_TIPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_SCPPC,		CFG_PRF_SCPPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_SCPPS,		CFG_PRF_SCPPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_BASC,		CFG_PRF_BASC))
            $(eval $(call MacroDefine,	BLE_PROFILE_BASS,		CFG_PRF_BASS))
            $(eval $(call MacroDefine,	BLE_PROFILE_HOGPD,		CFG_PRF_HOGPD))
            $(eval $(call MacroDefine,	BLE_PROFILE_HOGPBH,		CFG_PRF_HOGPBH))
            $(eval $(call MacroDefine,	BLE_PROFILE_HOGPRH,		CFG_PRF_HOGPRH))
            $(eval $(call MacroDefine,	BLE_PROFILE_GLPC,		CFG_PRF_GLPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_GLPS,		CFG_PRF_GLPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_RSCPC,		CFG_PRF_RSCPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_RSCPS,		CFG_PRF_RSCPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_CSCPC,		CFG_PRF_CSCPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_CSCPS,		CFG_PRF_CSCPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_CPPC,		CFG_PRF_CPPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_CPPS,		CFG_PRF_CPPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_LANC,		CFG_PRF_LANC))
            $(eval $(call MacroDefine,	BLE_PROFILE_LANS,		CFG_PRF_LANS))
            $(eval $(call MacroDefine,	BLE_PROFILE_ANPC,		CFG_PRF_ANPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_ANPS,		CFG_PRF_ANPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_PASPC,		CFG_PRF_PASPC))
            $(eval $(call MacroDefine,	BLE_PROFILE_PASPS,		CFG_PRF_PASPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_IPSC,		CFG_PRF_IPSC))
            $(eval $(call MacroDefine,	BLE_PROFILE_IPSS,		CFG_PRF_IPS))
            $(eval $(call MacroDefine,	BLE_PROFILE_ENVC,		CFG_PRF_ENVC))
            $(eval $(call MacroDefine,	BLE_PROFILE_ENVS,		CFG_PRF_ENVS))
            $(eval $(call MacroDefine,	BLE_PROFILE_WSCC,		CFG_PRF_WSCC))
            $(eval $(call MacroDefine,	BLE_PROFILE_WSCS,		CFG_PRF_WSCS))
            $(eval $(call MacroDefine,	BLE_PROFILE_BCSC,		CFG_PRF_BCSC))
            $(eval $(call MacroDefine,	BLE_PROFILE_BCSS,		CFG_PRF_BCSS))
            $(eval $(call MacroDefine,	BLE_PROFILE_UDSS,		CFG_PRF_UDSS))
            $(eval $(call MacroDefine,	BLE_PROFILE_UDSC,		CFG_PRF_UDSC))
            $(eval $(call MacroDefine,	BLE_PROFILE_WPTS,		CFG_PRF_WPTS))
            $(eval $(call MacroDefine,	BLE_PROFILE_WPTC,		CFG_PRF_WPTC))
            $(eval $(call MacroDefine,	BLE_PROFILE_PLXS,		CFG_PRF_PLXS))
            $(eval $(call MacroDefine,	BLE_PROFILE_PLXC,		CFG_PRF_PLXC))
            $(eval $(call MacroDefine,	BLE_PROFILE_CGMS,		CFG_PRF_CGMS))
            $(eval $(call MacroDefine,	BLE_PROFILE_CGMC,		CFG_PRF_CGMC))			
    	
    	endif #CONFIG_BLE_PROFILES
    
    	ifeq ($(CONFIG_BLE_MESH),y)
    		ifeq ($(CONFIG_BLE_PROFILES),)
    			CMARCODEFINES+=-DCFG_PRF -DCFG_NB_PRF=1
    		endif
    		CMARCODEFINES+=-DCFG_BLE_MESH -DCFG_BLE_MESH_MSG_API -DCFG_BLE_ADV_TEST_MODE
    
            $(eval $(call MacroDefine,	ECR_BLE_DBG,		CFG_BLE_MESH_DBG))
            $(eval $(call MacroDefine,	BLE_MESH_RELAY,				CFG_BLE_MESH_RELAY))
            $(eval $(call MacroDefine,	BLE_MESH_GATT_PROXY,		CFG_BLE_MESH_GATT_PROXY))
            $(eval $(call MacroDefine,	BLE_MESH_GATT_PROV,			CFG_BLE_MESH_GATT_PROV))
            $(eval $(call MacroDefine,	BLE_MESH_FRIEND,			CFG_BLE_MESH_FRIEND))
            $(eval $(call MacroDefine,	BLE_MESH_LPN,				CFG_BLE_MESH_LPN))
            $(eval $(call MacroDefine,	BLE_MESH_PROVER,			CFG_BLE_MESH_PROVER))
            $(eval $(call MacroDefine,	BLE_MESH_PROVEE,			CFG_BLE_MESH_PROVEE))
            $(eval $(call MacroDefine,	BLE_MESH_STORAGE_NONE,	CFG_BLE_MESH_STORAGE_NONE))	
            $(eval $(call MacroDefine,	BLE_MESH_STORAGE_WVT,	CFG_BLE_MESH_STORAGE_WVT))	
            $(eval $(call MacroDefine,	BLE_MESH_STORAGE_NVDS,	CFG_BLE_MESH_STORAGE_NVDS))
            $(eval $(call MacroDefine,	BLE_MESH_MDL_GENS,		CFG_BLE_MESH_MDL_GENS 		CFG_BLE_MESH_MDL_SERVER))
            $(eval $(call MacroDefine,	BLE_MESH_MDL_GENC,		CFG_BLE_MESH_MDL_GENC 		CFG_BLE_MESH_MDL_CLIENT))		
            $(eval $(call MacroDefine,	BLE_MESH_MDL_LIGHTS,	CFG_BLE_MESH_MDL_LIGHTS 	CFG_BLE_MESH_MDL_SERVER))
            $(eval $(call MacroDefine,	BLE_MESH_MDL_LIGHTC,	CFG_BLE_MESH_MDL_LIGHTC 	CFG_BLE_MESH_MDL_CLIENT))		
    	endif #CONFIG_BLE_MESH
		
		ifeq ($(CONFIG_TRC_ENABLE),y)
                        $(eval $(call MacroDefine,			TRC_HCI,					CFG_TRC_HCI))
                        $(eval $(call MacroDefine,			TRC_L2CAP,					CFG_TRC_L2CAP))
		endif #CONFIG_TRC_ENABLE
    
    endif #CONFIG_BLE_HOST_PRESENT
    
    $(eval $(call MacroDefine,			BLE_NVDS,				CFG_NVDS))
    $(eval $(call MacroDefine,			ECR_BLE_DBG,		CFG_BLE_DBG))
    $(eval $(call MacroDefine,			BLE_BOARD_DISPLAY,		CFG_DISPLAY))
    $(eval $(call MacroDefine,			BLE_MBP,				CFG_MBP))
    $(eval $(call MacroDefine,			RP_HWSIM_BYPASS,		RP_HWSIM_BYPASS))
    $(eval $(call MacroDefine,			BLE_APP_HT,				CFG_APP 	CFG_APP_PRF 	CFG_APP_HT 		CFG_APP_DIS))
    $(eval $(call MacroDefine,			BLE_APP_HID,			CFG_APP 	CFG_APP_PRF 	CFG_APP_HID 	CFG_APP_DIS 	CFG_PRF_BASS))
    $(eval $(call MacroDefine,			BLE_APP_ECR,		CFG_APP 	CFG_APP_PRF 	CFG_APP_ECR	CFG_PRF_ECR))
    $(eval $(call MacroDefine,			BLE_APP_AUDIO,			CFG_APP 	CFG_APP_PRF 	CFG_APP_SEC 	CFG_APP_AM0 	CFG_APP_DIS))
    $(eval $(call MacroDefine,			BLE_APP_MESH,			CFG_APP 	CFG_APP_MESH)) 
    $(eval $(call MacroDefine,			BLE_RTC,				CFG_RTC 	CFG_APP_TIME 	CFG_PRF_TIPC))
    $(eval $(call MacroDefine,			BLE_PS2,				CFG_PS2))
    
    $(eval $(call MacroDefine,			BLE_HCITL,				CFG_HCITL))
    $(eval $(call MacroDefine,			BLE_PRODUCT_SPLITEMB,	CFG_HCITL))
	
	ifeq ($(CONFIG_TRC_ENABLE),y)
                CMARCODEFINES+=-DCFG_TRC_EN 
                $(eval $(call MacroDefine,			TRC_KE_MSG,				CFG_TRC_KE_MSG))
                $(eval $(call MacroDefine,			TRC_KE_TMR,				CFG_TRC_KE_TMR))
                $(eval $(call MacroDefine,			TRC_KE_EVT,				CFG_TRC_KE_EVT))
                $(eval $(call MacroDefine,			TRC_MEM,				CFG_TRC_MEM))
                $(eval $(call MacroDefine,			TRC_SW_ASS,				CFG_TRC_SW_ASS))
                $(eval $(call MacroDefine,			TRC_CUSTOM,				CFG_TRC_CUSTOM))
	endif #CONFIG_TRC_ENABLE
	
	ifeq ($(CONFIG_LOG_ENABLE),y)
                $(eval $(call MacroDefine,			LOG_DEBUG,				CFG_LOG_DEBUG))
                $(eval $(call MacroDefine,			LOG_INFO,				CFG_LOG_INFO))
                $(eval $(call MacroDefine,			LOG_ERROR,				CFG_LOG_ERROR))
	endif #CONFIG_LOG_ENABLE
	
	ifeq ($(CONFIG_HCI_DBG_ENABLE),y)
                $(eval $(call MacroDefine,			DBG_HCI_CMD,				CFG_DBG_HCI_CMD))
                $(eval $(call MacroDefine,			DBG_HCI_ACL,				CFG_DBG_HCI_ACL))
                $(eval $(call MacroDefine,			DBG_HCI_SCO,				CFG_DBG_HCI_SCO))
                $(eval $(call MacroDefine,			DBG_HCI_EVENT,				CFG_DBG_HCI_EVENT))
	endif #CONFIG_HCI_DBG_ENABLE
	
    
endif #CONFIG_ECR_BLE




##################################################################

#WIFI ECR6600 Macro

##################################################################

ifeq ($(CONFIG_ECR6600_WIFI),y)

    $(eval $(call MacroDefine,	ARCH_ANDES,	CFG_NDS32))
    
    CMARCODEFINES+=-DCFG_VIRTEX7
    
    CMARCODEFINES+=-DCFG_WIFI_EMB -DCFG_SPLIT

    
    $(eval $(call MacroDefine,	WIFI_UMAC,			CFG_UMAC))
    $(eval $(call MacroDefine,	WIFI_FHOST,			CFG_FHOST))
#    $(eval $(call MacroDefine,	WIFI_PROF,					CFG_PROF))
#    $(eval $(call MacroDefine,	WIFI_STATS,					CFG_STATS))
    
    CMARCODEFINES+=-DCFG_WIFI_DBG=$(CONFIG_ECR6600_WIFI_DBG_LEVEL)
    
    $(eval $(call MacroDefine,	WIFI_BCN,					CFG_BCN))
    $(eval $(call MacroDefine,	WIFI_AGG,					CFG_AGG))
    $(eval $(call MacroDefine,	WIFI_LMAC_TEST,				NX_ESWIN_LMAC_TEST))
    
    
    CMARCODEFINES+=-DCFG_AMSDU_$(CONFIG_WIFI_AMSDURX)K
    
    CMARCODEFINES+=-DCFG_VIF_MAX=$(CONFIG_WIFI_VIF)
    CMARCODEFINES+=-DCFG_STA_MAX=$(CONFIG_WIFI_STA)
    CMARCODEFINES+=-DCFG_SPC=$(CONFIG_WIFI_SPC)
    CMARCODEFINES+=-DCFG_TXDESC0=$(CONFIG_WIFI_TXDESC0)
    CMARCODEFINES+=-DCFG_TXDESC1=$(CONFIG_WIFI_TXDESC1)
    CMARCODEFINES+=-DCFG_TXDESC2=$(CONFIG_WIFI_TXDESC2)
    CMARCODEFINES+=-DCFG_TXDESC3=$(CONFIG_WIFI_TXDESC3)
    CMARCODEFINES+=-DCFG_TXDESC4=$(CONFIG_WIFI_TXDESC4)
    CMARCODEFINES+=-DCFG_MAC_VER_V$(CONFIG_WIFI_MAC_VERSION)
    CMARCODEFINES+=-DCFG_MDM_VER_V$(CONFIG_WIFI_MDM_VERSION)
    CMARCODEFINES+=-DCFG_IPC_VER_V$(CONFIG_WIFI_IPC_VERSION)
    
    $(eval $(call MacroDefine,	WIFI_MDM_V10,				CFG_PLF_VER_V10))
    $(eval $(call MacroDefine,	WIFI_MDM_V11,				CFG_PLF_VER_V10))	
    $(eval $(call MacroDefine,	WIFI_MDM_V20,				CFG_PLF_VER_V20))	
    $(eval $(call MacroDefine,	WIFI_MDM_V21,				CFG_PLF_VER_V20))	
    $(eval $(call MacroDefine,	WIFI_MDM_V22,				CFG_PLF_VER_V20))	
    $(eval $(call MacroDefine,	WIFI_MDM_V30,				CFG_PLF_VER_V30))
    
    
    
    
    ifeq ($(CONFIG_PSM_SURPORT),y)
    
        $(eval $(call MacroDefine,	WIFI_POWERSVAE, 			CFG_PS))

    endif #CONFIG_PSM_SURPORT
    
    $(eval $(call MacroDefine,	WIFI_BFMEE, 				CFG_BFMEE))
    
    $(eval $(call MacroDefine,	WIFI_MURX, 					CFG_MU_RX))
    
    $(eval $(call MacroDefine,	WIFI_BFMER, 				CFG_BFMER))
    
    ifeq ($(CONFIG_WIFI_BFMER),y)
        CMARCODEFINES+=-DCFG_MU_CNT=$(CONFIG_WIFI_MUCNT)
    endif
    
    $(eval $(call MacroDefine,	WIFI_DPSM, 					CFG_DPSM))
    
    $(eval $(call MacroDefine,	WIFI_UAPSD, 				CFG_UAPSD))
    
    $(eval $(call MacroDefine,	WIFI_CMON, 					CFG_CMON))
    
    $(eval $(call MacroDefine,	WIFI_MROLE, 				CFG_MROLE))
    
    $(eval $(call MacroDefine,	WIFI_HWSCAN, 				CFG_HWSCAN))
    
    $(eval $(call MacroDefine,	WIFI_AUTOBCN, 				CFG_AUTOBCN))
    
    $(eval $(call MacroDefine,	WIFI_KEYCFG, 				CFG_KEYCFG))
    
    CMARCODEFINES+=-DCFG_P2P=$(CONFIG_WIFI_P2P)
    
    $(eval $(call MacroDefine,	WIFI_P2P_GO, 				CFG_P2P_GO))
    
    $(eval $(call MacroDefine,	WIFI_WAPI, 					CFG_WAPI))
    
    $(eval $(call MacroDefine,	WIFI_BWLEN, 				CFG_BWLEN))
    
    ifneq ($(CONFIG_WIFI_MESH_VIF),0)
    
    	CMARCODEFINES+=-DCFG_MESH
    
    	CMARCODEFINES+=-DCFG_MESH_VIF=$(CONFIG_WIFI_MESH_VIF)
    
    	CMARCODEFINES+=-DCFG_MESH_LINK=$(CONFIG_WIFI_MESH_LINK)
    
    	CMARCODEFINES+=-DCFG_MESH_PATH=$(CONFIG_WIFI_MESH_PATH)
    
    	CMARCODEFINES+=-DCFG_MESH_PROXY=$(CONFIG_WIFI_MESH_PROXY)
    	
    endif
    
#    $(eval $(call MacroDefine,	WIFI_VHT, 					CFG_VHT))
    
    $(eval $(call MacroDefine,	WIFI_HE, 					CFG_HE))
    $(eval $(call MacroDefine,  WIFI_TWT,                                        CFG_TWT))
    CMARCODEFINES+=-DCFG_BARX=$(CONFIG_WIFI_BARX)
    	
    CMARCODEFINES+=-DCFG_BATX=$(CONFIG_WIFI_BATX)
    	
    CMARCODEFINES+=-DCFG_REORD_BUF=$(CONFIG_WIFI_REORDBUF)
    
    $(eval $(call MacroDefine,	WIFI_AMSDU, 				CFG_AMSDU))
    
#    $(eval $(call MacroDefine,	WIFI_DBGDUMP, 				CFG_DBGDUMP))
    
#    $(eval $(call MacroDefine,	WIFI_DBGDUMPKEY, 			CFG_DBGDUMPKEY))
    
#    $(eval $(call MacroDefine,	WIFI_TRACE, 				CFG_TRACE))
    
    $(eval $(call MacroDefine,	WIFI_REC, 					CFG_REC))
    
    $(eval $(call MacroDefine,	WIFI_UF, 					CFG_UF))
    
    
    $(eval $(call MacroDefine,	WIFI_MON_DATA, 				CFG_MON_DATA))
    
    
    $(eval $(call MacroDefine,	WIFI_TDLS, 					CFG_TDLS))
    
    CMARCODEFINES+=-DCFG_HSU=$(CONFIG_WIFI_HSU)
    
    $(eval $(call MacroDefine,	ESWIN_SDIO, 				NX_ESWIN_SDIO))



endif #CONFIG_ECR6600_WIFI

macro:
	@echo $(CMARCODEFINES)
