#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>
#include <utility/w5100.h>

//----------------------------------------------------------------------------------------
//2WG EEPROM DEFINES
//----------------------------------------------------------------------------------------

#define E_Login_Password_2 0
#define E_Alarm_Default_3 7
#define E_Email_Default_4 9
#define E_Daylight_Saving_5 11
#define E_Periodic_RAM_Reporting_6 13
#define E_Alarm_Automation_7 15
#define E_RAM_Checking_8 17
#define E_Garage_Door_Default_9 19
#define E_Heating_Mode_10 21
#define E_Cooling_Mode_11 23
#define E_START_UP_12 25
#define E_Serial_Start_13 34
#define E_Ethernet_OK_14 47
#define E_SD_Card_OK_15 59
#define E_SD_Card_Failure_16 70
#define E_0000_17 86
#define E_UDP_NTP_OK_18 91
#define E_Backup_Reload_OK_19 102
#define E_Alarm_Active_20 119
#define E_Alarm_Disabled_21 132
#define E_Garage_Door_Web_Active_22 147
#define E_Garage_Door_Web_Disabled_23 170
#define E_Free_RAM__24 195
#define E_Socket_Num__26 205
#define E_ETHERNET_SOCKET_LIST_27 217
#define E_Socket_Heading_28 238
#define E_Start_Up_Complete_29 271
#define E_Climate_Period_Set__30 289
#define E_Air_Pump_Switched_On_31 309
#define E_Air_Pump_Switched_Off_32 330
#define E_Alarm_Switched_On_34 352
#define E_Date__35 370
#define E_Time__36 376
#define E_Ethernet_Socket_Status_37 382
#define E_File_Upload_Form_38 405
#define E_File_Upload_39 422
#define E_Garage_Door_Activated_41 434
#define E_Garage_Door_Deactivated_42 456
#define E_Garage_Door_Operated_43 480
#define E_Alarm_Activated_44 501
#define E_Alarm_Deactivated_45 517
#define E_GET_46 535
#define E_POST_47 539
#define E_HTTP_48 544
#define E_icon_50 549
#define E_Email__56 554
#define E_Email_OK_57 561
#define E_Alarm_Alert_58 570
#define E_Garage_Closing_59 582
#define E_Garage_Open_60 597
#define E_Garage_Opening_61 609
#define E_Garage_Closed_62 624
#define E_Auto_Garage_Close_Buzzer_63 638
#define E_Garage_Door_is_Open_65 663
#define E_GARAGE_DOOR_OPEN_66 683
#define E_Climate_Update_67 700
#define E_Daily_Climate_Update_68 715
#define E_DASHBOARD_69 736
#define E_CURRENT_RAM_70 746
#define E_RECENT_CLIMATE_71 758
#define E_Temperature_72 773
#define E_Humidity_73 785
#define E_CLIMATE_FOR_WEEK_74 794
#define E_BATHROOM_75 811
#define E_SECURITY_76 820
#define E_SETTINGS_77 829
#define E_SD_CARD_78 838
#define E_SD_FILE_DISPLAY_79 846
#define E_INPUTPASSWORD_80 862
#define E_Password_Data_Entry_81 876
#define E_Enter_Password__82 896
#define E_INPUT_83 912
#define E_SD_Card_File_Count__84 918
#define E_tbTime__86 938
#define E_Garage_Door_Default_90 946
#define E_Email_Default_91 966
#define E_Daylight_Saving_92 980
#define E_Periodic_RAM_Reporting_93 996
#define E__setting_switched_to__94 1019
#define E_Lounge_95 1041
#define E_Bedroom_96 1048
#define E_Hallway_97 1056
#define E_Garage_98 1064
#define E_Patio_99 1071
#define E_Bathroom_100 1077
#define E_MAX_RAM_DATA_SIZE_101 1086
#define E_MAX_RAM_STACK_SIZE_102 1104
#define E_MAX_RAM_HEAP_SIZE_103 1123
#define E_MINIMUM_FREE_RAM_104 1141
#define E_MAXIMUM_FREE_HEAP_105 1158
#define E_tbStatistic__107 1176
#define E_tbFree_Heap_List__108 1189
#define E_13_tildes_109 1207
#define E_File_Not_Found__110 1221
#define E_SENSORco__111 1237
#define E_tbTEMP_NOWco__113 1246
#define E_tbMAX_TEMPco__114 1259
#define E_NULL_123 1272
#define E_Alarm_Default_124 1277
#define E_HOSTco__127 1291
#define E_COOKIEco__128 1298
#define E_SIDeq_129 1307
#define E_Garage_Door_Not_ActivetbGarage_Door_is_Open_130 1312
#define E_tbTemptbMax_131 1356
#define E_tbTemptbMin_132 1368
#define E_tbHumiditytbMax_133 1380
#define E_tbHumiditytbMin_134 1396
#define E_BACKUPSsl_135 1412
#define E_HIST_136 1421
#define E_Dateco__137 1426
#define E_Temp_hy_Max_138 1433
#define E_Temp_hy_Min_139 1444
#define E_Humidity_hy_Max_140 1455
#define E_Humidity_hy_Min_141 1470
#define E_ACTIVITYsl_142 1485
#define E_HTMLREQUsl_143 1495
#define E_HACKLOGSsl_144 1505
#define E_Temp_145 1515
#define E_helo__146 1520
#define E_MAIL_Fromco__147 1526
#define E_RCPT_Toco__148 1538
#define E_Fromco__149 1548
#define E_ACTV_151 1555
#define E_HTMLREQU_152 1560
#define E_HTML_153 1569
#define E_HACKLOGS_154 1574
#define E_HACK_155 1583
#define E_PIR_Sensorco__156 1588
#define E_tbTimeco__158 1601
#define E_ALARM_PIR_ALERT_159 1610
#define E_ALRMTIMEdtTXT_160 1626
#define E_Auto_Alarm_ON_162 1639
#define E_Auto_Alarm_OFF_163 1653
#define E_tbCheckPoint__164 1668
#define E_CHECKRAMdtTXT_166 1682
#define E_Alarm_Times_Loaded_167 1695
#define E_RAM_Checking_is_On_168 1714
#define E_RAM_Checking_is_Off_169 1733
#define E_File_Record_Length_Error_172 1753
#define E_Dashboard_173 1783
#define E_Recent_Climate_174 1793
#define E_Climate_For_Week_175 1808
#define E_Security_177 1825
#define E_SD_Card_Display_178 1834
#define E_SD_File_Display_179 1850
#define E_Garage_Door_Activate_180 1866
#define E_Garage_Door_Operate_181 1887
#define E_5_Min_Climate_182 1907
#define E_15_Min_Climate_183 1921
#define E_30_Min_Climate_184 1936
#define E_60_Min_Climate_185 1951
#define E_BslR_Light_Activate_186 1966
#define E_BslR_Fan_Activate_187 1985
#define E_BslR_Heater_Activate_188 2002
#define E_Password_Login_190 2022
#define E_Alarm_Activate_191 2037
#define E_Settings_192 2052
#define E_Alarm_Dflt_193 2061
#define E_Garage_Door_Dflt_194 2072
#define E_Email_Dflt_195 2089
#define E_Email_Activate_196 2100
#define E_RAM_Usage_197 2115
#define E_Periodic_Ram_Rptg_199 2125
#define E_Alarm_Automation_200 2143
#define E_RAM_Checking_201 2160
#define E_Local_Garage_Door_Open_202 2173
#define E_Local_Alarm_Off_203 2196
#define E_Dump_File_To_Browser_204 2212
#define E_Test_Email_205 2233
#define E_SEND_TEST_EMAIL_206 2244
#define E_Test_email_Complete_208 2260
#define E_Automated_Heating_210 2280
#define E_Automated_Cooling_211 2298
#define E_HOME_HEATING_212 2316
#define E_Switch_Air_Pump_On_213 2329
#define E_Switch_Air_Pump_Off_214 2348
#define E_UDP_NTP_Update_216 2368
#define E_Local_Alarm_On_217 2383
#define E_Local_Garage_Door_Close_218 2398
#define E_400_Bad_Request_219 2422
#define E_BAD_HTML_REQUEST_221 2438
#define E_Socket_Num_222 2461
#define E_Dest_Port_223 2474
#define E_PROHIBITED_REQUEST_224 2488
#define E_IP_ADDRESS_BANNED_227 2513
#define E_Bad_Page_Number_228 2537
#define E_HOST_230 2559
#define E_PASSWORD_231 2564
#define E_COOKIE_232 2573
#define E_BOUNDARY_233 2580
#define E_LENGTH_234 2589
#define E_FOLDER_235 2596
#define E_REQUEST_236 2603
#define E_SMTPServer_238 2611
#define E_FILEDELETE_239 2630
#define E_FNAME_240 2641
#define E_Failed_HTML_Requests_241 2647
#define E_INVALID_FILE_ACCESS_ATTEMPT_246 2668
#define E_Free_Heap_Sizeco__247 2702
#define E_Free_Heap_Countco__248 2719
#define E_slIMAGESslLOC_ICONdtPNGsl_249 2737
#define E_ROBOTSdtTXT_250 2759
#define E_slIMAGESslWWW_ICONdtPNG_251 2770
#define E_hy_REQUESTco__252 2791
#define E_Memory_Backup_253 2803
#define E_Statistics_254 2817
#define E_hy_PAGEco__255 2828
#define E_Websiteco__256 2837
#define E_Requestco__257 2847
#define E_IP_Addressco__258 2857
#define E_Socket_hsco__259 2870
#define E_Dest_Portco__260 2881
#define E_PASSWORDco__261 2893
#define E_Upload_timeout_262 2904
#define E_hy_Data_Length_263 2919
#define E_Filenameco_264 2933
#define E_USERhyAGENTco_265 2943
#define E_CONTENThyTYPEco_266 2955
#define E_MULTIPART_267 2969
#define E_BOUNDARYeq_268 2979
#define E_CONTENThyLENGTHco_269 2989
#define E_404_Not_Found_270 3005
#define E_ContenthyTypeco__271 3019
#define E_textslplain_272 3034
#define E_imagesljpeg_273 3045
#define E_imageslpng_274 3056
#define E_applicationslpdf_275 3066
#define E_applicationslvnddtmshyexcel_276 3082
#define E_applicationslmsword_277 3107
#define E_200_OK_278 3126
#define E__HOME_AUTOMATION_279 3133
#define E_UPLOAD_281 3150
#define E_slFLASHdtTXT_282 3157
#define E_ETHERNET_SOCKETS_283 3168
#define E_STATISTICS_285 3185
#define E_STATSdtTXT_286 3196
#define E_FILE_UPLOAD_287 3206
#define E_hy_Free_RAMco__288 3218
#define E_hy_Loop_Heap_SizeslFreeslCountco__289 3231
#define E_Reload__290 3261
#define E_AIRPUMP_291 3269
#define E__hy_Disconnected_292 3277
#define E_NAMEeqqtFOLDERqt_293 3293
#define E_FILENAMEeq_294 3307
#define E_hy_Browser_IPco__295 3317
#define E_IPADDRESS_296 3332
#define E_WEB_CRAWLERS_297 3342
#define E_CRAWLER_298 3355
#define E_ERROR_PAGE_299 3363
#define E_browserconfigdtxml_300 3374
#define E_FROMco_301 3392
#define E_DOWNTHEMALL_302 3398
#define E_hy_Cookie_Deleted_303 3410
#define E_SCKT_DISCco_304 3427
#define E_PUBLICsl_305 3438
#define E_IMAGESsl_306 3446
#define E_SCRNINFOsl_308 3454
#define E_asas_HTML_REQUEST_asas_309 3464
#define E_Activity_310 3483
#define E_Crawler_311 3492
#define E_Process_IP_Address_312 3500
#define E_XSS_KeyWords_313 3519
#define E_SUPPRESSION_314 3626
#define E_HEAD_315 3638
#define E_HTML_Process_Termination_316 3643
#define E_Duration_317 3668
#define E_Resource_318 3677
#define E_Benefit_319 3686
#define E_HACKER_ACTIVITY_320 3694
#define E_CRAWLER_ACTIVITY_321 3710
#define E_ERROR_CaptureHeatingsStats_322 3727
#define E_Baseline_323 3754
#define E_HEATING_324 3763
#define E_COOLING_325 3771
#define E_Hacker_326 3779
#define E_Catweazle_327 3786
#define E_Public_328 3796
#define E_applicationslzip_329 3803
#define E_MOBILE_330 3819
#define E_REFERERco_331 3826
#define E_ARDUINO_332 3835

#define GC_MinFreeRAM  1200

//----------------------------------------------------------------------------------------
//GTM EEPROM DEFINES
//----------------------------------------------------------------------------------------
/*
#define E_Login_Password_2 0
#define E_Alarm_Default_3 7
#define E_Email_Default_4 9
#define E_Daylight_Saving_5 11
#define E_Periodic_RAM_Reporting_6 13
#define E_Alarm_Automation_7 15
#define E_RAM_Checking_8 17
#define E_START_UP_12 19
#define E_Serial_Start_13 28
#define E_Ethernet_OK_14 41
#define E_SD_Card_OK_15 53
#define E_SD_Card_Failure_16 64
#define E_0000_17 80
#define E_UDP_NTP_OK_18 85
#define E_Backup_Reload_OK_19 96
#define E_Alarm_Active_20 113
#define E_Alarm_Disabled_21 126
#define E_Free_RAM__24 141
#define E_SOCKET_DISCONNECTION_25 151
#define E_Socket_Num__26 172
#define E_ETHERNET_SOCKET_LIST_27 184
#define E_Socket_Heading_28 205
#define E_Start_Up_Complete_29 238
#define E_Climate_Period_Set__30 256
#define E_Alarm_Switched_On_34 276
#define E_Date__35 294
#define E_Time__36 300
#define E_Ethernet_Socket_Status_37 306
#define E_File_Upload_Form_38 329
#define E_File_Upload_39 346
#define E_Analysis_Result_40 358
#define E_Alarm_Activated_44 374
#define E_Alarm_Deactivated_45 390
#define E_GET_46 408
#define E_POST_47 412
#define E_HTTP_48 417
#define E_DEL_49 422
#define E_icon_50 426
#define E_Password_OK_51 431
#define E_New_52 443
#define E_Password_NOK_53 447
#define E_Cookie_OK_54 460
#define E_Cookie_Not_OK_55 470
#define E_Email__56 484
#define E_Email_OK_57 491
#define E_Alarm_Alert_58 500
#define E_Climate_Update_67 512
#define E_Daily_Climate_Update_68 527
#define E_DASHBOARD_69 548
#define E_CURRENT_RAM_70 558
#define E_RECENT_CLIMATE_71 570
#define E_Temperature_72 585
#define E_Humidity_73 597
#define E_CLIMATE_FOR_WEEK_74 606
#define E_SECURITY_76 623
#define E_SETTINGS_77 632
#define E_SD_CARD_78 641
#define E_SD_FILE_DISPLAY_79 649
#define E_INPUTPASSWORD_80 665
#define E_Password_Data_Entry_81 679
#define E_Enter_Password__82 699
#define E_INPUT_83 715
#define E_SD_Card_File_Count__84 721
#define E_MIN_FREE_RAM_tbProc__85 741
#define E_tbTime__86 770
#define E_tbMin_Free_RAM__87 778
#define E_EXPIRED_88 794
#define E_LIMIT_89 802
#define E_Email_Default_91 808
#define E_Daylight_Saving_92 822
#define E_Periodic_RAM_Reporting_93 838
#define E__setting_switched_to__94 861
#define E_Lounge_95 883
#define E_Bedroom_96 890
#define E_Hallway_97 898
#define E_Garage_98 906
#define E_Patio_99 913
#define E_Bathroom_100 919
#define E_MAX_RAM_DATA_SIZE_101 928
#define E_MAX_RAM_STACK_SIZE_102 946
#define E_MAX_RAM_HEAP_SIZE_103 965
#define E_MINIMUM_FREE_RAM_104 983
#define E_MAXIMUM_FREE_HEAP_105 1000
#define E_tbParent_Procedure__106 1018
#define E_tbStatistic__107 1038
#define E_tbFree_Heap_List__108 1051
#define E_13_tildes_109 1069
#define E_File_Not_Found__110 1083
#define E_SENSORco__111 1099
#define E_tbPREV_TEMPco__112 1108
#define E_tbTEMP_NOWco__113 1122
#define E_tbMAX_TEMPco__114 1135
#define E_slSDCardsl_115 1148
#define E_slSDFilesl_116 1157
#define E_tbRequest_Header_Size__117 1166
#define E_HACK_ATTEMPT_118 1189
#define E_HOST__119 1202
#define E_tbPASSWORDco__120 1208
#define E_tbHOST__121 1221
#define E_INVALID_PASSWORD_122 1229
#define E_NULL_123 1246
#define E_Alarm_Default_124 1251
#define E_PASSWORDeq_125 1265
#define E_slINPUTPASSWORDsl_126 1275
#define E_HOSTco__127 1291
#define E_COOKIEco__128 1298
#define E_SIDeq_129 1307
#define E_tbTemptbMax_131 1312
#define E_tbTemptbMin_132 1324
#define E_tbHumiditytbMax_133 1336
#define E_tbHumiditytbMin_134 1352
#define E_BACKUPSsl_135 1368
#define E_HIST_136 1377
#define E_Dateco__137 1382
#define E_Temp_hy_Max_138 1389
#define E_Temp_hy_Min_139 1400
#define E_Humidity_hy_Max_140 1411
#define E_Humidity_hy_Min_141 1426
#define E_ACTIVITYsl_142 1441
#define E_HTMLREQUsl_143 1451
#define E_HACKLOGSsl_144 1461
#define E_Temp_145 1471
#define E_helo__146 1476
#define E_MAIL_Fromco__147 1482
#define E_RCPT_Toco__148 1494
#define E_Fromco__149 1504
//#define E_ACTIVITY_150 1511
#define E_ACTV_151 1520
#define E_HTMLREQU_152 1525
#define E_HTML_153 1534
#define E_HACKLOGS_154 1539
#define E_HACK_155 1548
#define E_PIR_Sensorco__156 1553
#define E_tbDateco__157 1566
#define E_tbTimeco__158 1575
#define E_ALARM_PIR_ALERT_159 1584
#define E_ALRMTIMEdtTXT_160 1600
#define E_ALARM_ACTION_TIMES_161 1613
#define E_Auto_Alarm_ON_162 1632
#define E_Auto_Alarm_OFF_163 1646
#define E_tbCheckPoint__164 1661
#define E_RAM_Checking_165 1675
#define E_CHECKRAMdtTXT_166 1688
#define E_Alarm_Times_Loaded_167 1701
#define E_RAM_Checking_is_On_168 1720
#define E_RAM_Checking_is_Off_169 1739
#define E_Push_Pop_Error_170 1759
#define E_Procedure__171 1774
#define E_File_Record_Length_Error_172 1785
#define E_Dashboard_173 1815
#define E_Recent_Climate_174 1825
#define E_Climate_For_Week_175 1840
#define E_Security_177 1857
#define E_SD_Card_Display_178 1866
#define E_SD_File_Display_179 1882
#define E_5_Min_Climate_182 1898
#define E_15_Min_Climate_183 1912
#define E_30_Min_Climate_184 1927
#define E_60_Min_Climate_185 1942
//#define E_Enter_Password__189 1957
#define E_Password_Login_190 1973
#define E_Alarm_Activate_191 1988
#define E_Settings_192 2003
#define E_Alarm_Dflt_193 2012
#define E_Email_Dflt_195 2023
#define E_Email_Activate_196 2034
#define E_RAM_Usage_197 2049
//#define E_Daylight_Saving_198 2059
#define E_Periodic_Ram_Rptg_199 2075
#define E_Alarm_Automation_200 2093
#define E_RAM_Checking_201 2110
#define E_Local_Alarm_Off_203 2123
#define E_Dump_File_To_Browser_204 2139
#define E_Test_Email_205 2160
#define E_SEND_TEST_EMAIL_206 2171
#define E_Email_Initialise_OK_207 2187
#define E_Test_email_Complete_208 2207
#define E_Test_email_Failure_209 2227
#define E_UPD_NTP_hy_Start_215 2246
#define E_UDP_NTP_Update_216 2262
#define E_Local_Alarm_On_217 2277
#define E_400_Bad_Request_219 2292
#define E_GET_POST_and_HTTP_Required_220 2308
#define E_BAD_HTML_REQUEST_221 2335
#define E_Socket_Num_222 2358
#define E_Dest_Port_223 2371
#define E_PROHIBITED_REQUEST_224 2385
#define E_403_Forbidden_225 2410
#define E_IP_Address_is_Banned_226 2424
#define E_IP_ADDRESS_BANNED_227 2445
#define E_Bad_Page_Number_228 2469
#define E_Invalid_Request_Login_Required_229 2491
#define E_HOST_230 2524
#define E_PASSWORD_231 2529
#define E_COOKIE_232 2538
#define E_BOUNDARY_233 2545
#define E_LENGTH_234 2554
#define E_FOLDER_235 2561
#define E_REQUEST_236 2568
#define E_Office_237 2576
#define E_SMTPServer_238 2583
#define E_FILEDELETE_239 2602
#define E_FNAME_240 2613
#define E_Banned_IP_Addresses_241 2619
#define E_slALARMONsl_242 2639
#define E_slALARMOFFsl_243 2649
//#define E_slGARAGEOPENsl_244 2660
//#define E_slGARAGECLOSEsl_245 2673
#define E_INVALID_FILE_ACCESS_ATTEMPT_246 2687
#define E_Free_Heap_Sizeco__247 2721
#define E_Free_Heap_Countco__248 2738
#define E_slIMAGESslLOC_ICONdtPNGsl_249 2756
#define E_ROBOTSdtTXT_250 2778
#define E_slIMAGESslWWW_ICONdtPNG_251 2789
#define E_hy_REQUESTco__252 2810
#define E_Memory_Backup_253 2822
#define E_Statistics_254 2836
#define E_hy_PAGEco__255 2847
#define E_Websiteco__256 2856
#define E_Requestco__257 2866
#define E_IP_Addressco__258 2876
#define E_Socket_hsco__259 2889
#define E_Dest_Portco__260 2900
#define E_PASSWORDco__261 2912
#define E_Upload_timeout_262 2923
#define E_hy_Data_Length_263 2938
#define E_Filenameco_264 2952
#define E_USERhyAGENTco_265 2962
#define E_CONTENThyTYPEco_266 2974
#define E_MULTIPART_267 2988
#define E_BOUNDARYeq_268 2998
#define E_CONTENThyLENGTHco_269 3008
#define E_404_Not_Found_270 3024
#define E_ContenthyTypeco__271 3038
#define E_textslplain_272 3053
#define E_imagesljpeg_273 3064
#define E_imageslpng_274 3075
#define E_applicationslpdf_275 3085
#define E_applicationslvnddtmshyexcel_276 3101
#define E_applicationslmsword_277 3126
#define E_200_OK_278 3145
#define E__HOME_AUTOMATION_279 3152
#define E__MONITOR_280 3169
#define E_UPLOAD_281 3178
#define E_slFLASHdtTXT_282 3185
#define E_ETHERNET_SOCKETS_283 3196
#define E_BANNED_IP_ADDRESSES_284 3213
#define E_STATISTICS_285 3233
#define E_STATSdtTXT_286 3244
#define E_FILE_UPLOAD_287 3254
#define E_hy_Free_RAMco__288 3266
#define E_hy_Loop_Heap_SizeslFreeslCountco__289 3279
#define E_Reload__290 3309
#define E_AIRPUMP_291 3317
#define E__hy_Disconnected_292 3325
#define E_NAMEeqqtFOLDERqt_293 3341
#define E_FILENAMEeq_294 3355
#define E_hy_Browser_IPco__295 3365
#define E_IPADDRESS_296 3380
#define E_WEB_CRAWLERS_297 3390
#define E_CRAWLER_298 3403
#define E_ERROR_PAGE_299 3411

#define GC_MinFreeRAM  2200
*/
//------------------------------------------------------------------------------
//MESSAGE DEFINES
//------------------------------------------------------------------------------

#define M_Ethernet_Failure_1 1
#define M_UDP_NTP_Failure_2 2
#define M_Backup_Reload_Failure_3 3
#define M_Build_Time_Initialisation_4 4
#define M_Processor_Time_Rollover_5 5
#define M_UDP_NTP_Rollover_Failure_6 6
#define M_Home_Automation_Datetime_Rollover_Error_7 7
#define M_EMERGENCY_FIRE_ALERT_8 8
#define M_HIGH_TEMPERATURE_ALERT_9 9
#define M_File_Deleted__10 10
#define M_FILE_NOT_DELETED__11 11
#define M_Ethernet_Email_Not_Operational_12 12
#define M_ERROR_Email_Failure_13 13
#define M_Garage_Door_OPEN_AND_CLOSED_Fault_14 14
#define M_Sensor_Fault_and_Garage_Deactivated_15 15
#define M_Email_Currently_Disabled_16 16
#define M_File_Open_Error__17 17
#define M_FAULTY_CLIMATE_SENSOR_ALERT_18 18
#define M_Unable_to_Open_SD_Card_Root_19 19
#define M_Could_not_open_file__20 20
#define M_Unused_Email_Server_21 21
#define M_email_server_IP_22 22
#define M_email_recipient_23 23
#define M_email_sender_24 24
#define M_Call_Hierarchy_Overflow_25 25
#define M_Call_Hierarchy_Underflow_26 26
#define M_Climate_Sensor_Check_hy_All_OK_27 27
#define M_SD_Card_Error_hy_Climate_Recording_Skipped_28 28
#define M_Invalid_Password_29 29
#define M_401_Unauthorized_30 30
#define M_404_Not_Found_31 31
#define M_Invalid_Web_Page_Access_32 32
#define M_Invalid_Web_Page_33 33
#define M_Proxy_HTML_Request_Not_Accepted_34 34
#define M_File_Upload_Process_Error_ha_35 35
#define M_HOST_Request_Header_Field_is_Required_36 36
#define M_Invalid_URL_File_Extension_37 37
#define M_Partial_HTML_Request_Received_and_Ignored_38 38

//------------------------------------------------------------------------------

void FileMMDDWriteSPICSC(const String &p_directory, const String &p_filename_prefix, const String &p_message);

void SetDaylightSavingTime(boolean p_dst);
void SetStartDatetime(long p_startdatetime);
void SetStartDate(long p_startdate);
void SetStartTime(long p_starttime);
unsigned long Time();
unsigned long Now();
unsigned long Date();
unsigned long DateTime(unsigned long p_millis);

void CheckMillisRollover();
boolean CheckSecondsDelay(unsigned long &p_AnyTime, unsigned long p_MilliSecDelay);
long    DateExtract(const String &p_date);
long    DateEncode(int p_dd, int p_mm, int p_yyyy);
void    DateDecode(long p_datetime, int *p_dd, int *p_mm, int *p_yyyy);
int     DayOfWeekNum(long p_datetime); //Mon = 1, Sun = 7;
String  DateToString(long p_datetime);
void    TimeDecode(long p_datetime, int *p_hh, int *p_mm);
unsigned long TimeEncode(int p_hour, int p_min, int p_sec);
long    TimeExtract(const String &p_time);
String  TimeToHHMM(long p_time);
String  TimeToHHMMSS(long p_time);
String  DayOfWeek(long p_datetime); //MON, TUE, WED, etc

String ENDF2(const String &p_line, int &p_start, char p_delimiter);
String FormatAmt(double p_double, byte p_dec_plc);
double StringToDouble(String p_string);
String ZeroFill(long p_long, int p_length);

void SettingSwitch(const String &p_setting_desc, int p_setting);
String EPSR(int p_start_posn); //EEPROMStringRead
String EPSRU(int p_start_posn); //EEPROMStringReadUpperCase
String EPFR(int &p_start_posn, char p_delimiter); //EEPROMFieldRead
void EPSW(int p_start_posn, char p_string[]); //EEPROMStringWrite

int freeRam();
void  RamUsage(int &p_data_bss_size, int &p_heap_size, int &p_free_ram, int &p_stack_size, int &p_heap_free_size, int p_heap_list_sizes[]);
void AnalyseFreeHeap(int &p_heap_free_size, byte &p_heap_free_count, int p_heap_list_sizes[]);
void CheckRAM();

//These are the cache email procedures
void EmailInitialise(const String &p_heading);
void EmailLine(const String &p_line);
void EmailDisconnect();
//These are the transmission email procedures
//byte EmailInitialiseXM(const String &p_heading);
//void EmailLineXM(const String &p_line);
//void EmailDisconnectXM();

void SendTestEmail();
boolean CheckEmailResponse();

void TwoWayCommsSPICSC2(const String &p_message); //, byte p_email_init);

String DetermineFilename(const int p_type);

void ActivityWrite(const String &p_activity);
//void HackWrite(const String &p_html_request);
void HTMLWrite(const String &p_html_request);

void ActivityWriteXM(const String &p_activity);
//void HTMLWriteXM(const String &p_html_request);
//void HackWriteXM(const String &p_html_request);

int CurrentSPIDevice();
void SPIDeviceSelect(const int p_device);

void CheckForUPDNTPUpdate();
unsigned long sendNTPpacket(IPAddress& p_address);
boolean ReadTimeUDP();

void CheckSRAMCheckingStart();
void Pop(byte p_proc_num);
void Push(byte p_procedure_num);
String DirectFileRecordRead(File &p_checkram_file, int p_record_length, int p_proc_num);
void ResetSRAMStatistics();
void WriteSRAMAnalysisChgsSPICSC3();
void WriteSRAMStatisticSPICSC2(byte p_statistic);

String SRAMStatisticLabel(byte p_statistic);
void StatisticCount(int p_statistic, boolean p_excl_catweazle);
#define DC_StatisticsCount 30

#define St_Dashboard_WPE 0
#define St_RAM_Usage_WPE 1
#define St_Recent_Climate_WPE 2
#define St_Weekly_Climate_WPE 3
#define St_Home_Heating_WPE 4
#define St_Ethernet_Socket_WPE 5
#define St_Banned_IP_Addresses_WPE 6
#define St_Bathroom_WPE 7
#define St_Security_WPE 8
#define St_Settings_WPE 9
#define St_SD_Card_List_WPE 10
#define St_SD_Text_File_WPE 11
#define St_Statistics_WPE 12
#define St_Code_File_Downloads_E 13
#define St_Other_File_Downloads_E 14
#define St_File_Uploads 15
#define St_OK_Logins 16
#define St_Proxy_Hacks 17
#define St_Web_Page_Extension_Hacks 18
#define St_Direct_File_Hacks 19
#define St_Other_Hacks 20
//Cross site html hacks injectors??
#define St_Garage_Door_Operations 21
#define St_LAN_Page_Connects 22
#define St_WWW_Page_Connects 23
#define St_Banned_IP_Addr_Refusals 24
#define St_File_Cache_Operations 25
#define St_Socket_Disconnections 26
#define St_Discarded_ICON_Requests 27
#define St_Web_Crawler_WPE 28
#define St_Error_Page_WPE 29

#define DC_RAMDataCount   5
#define DC_RAMDataSize    0
#define DC_RAMStackSize   1
#define DC_RAMHeapSize    2
#define DC_RAMMinFree     3
#define DC_RAMHeapMaxFree 4
#define DC_CallHierarchyCount 15
#define DC_HeapListCount 6

struct TypeRAMData {
  byte CallHierarchy[DC_CallHierarchyCount];
  unsigned long TimeOfDay;
  int    Value;
  boolean Updated;
};

//These are our linked list cache record types
#define DC_Email_Initialise 0
#define DC_Email_Line 1
#define DC_Email_Disconnect 2
#define DC_Activity_Log 3
#define DC_Hack_Attempt_Log 4
#define DC_Crawler_Request_Log 5
#define DC_CWZ_Request_Log 6
#define DC_HTML_Request_Log 7

//Define the nodes for our linked list cache
typedef struct TCacheItem *PCacheItem;
struct TCacheItem {
  String ItemData;
  byte Type;
  PCacheItem Next;
};

typedef struct THTMLParm *PHTMLParm;
struct THTMLParm {
  int Name;
  String Value;
  PHTMLParm Next;
};

//SPI Interface stuff
const int DC_SDCardSSPin = 4;
const int DC_EthernetSSPin = 10;
const int DC_SPIHardwareSSPin = 53;
const int C_SPIDeviceCount = 3;
const int C_SPIDeviceSSList[] = {DC_SDCardSSPin, DC_EthernetSSPin,DC_SPIHardwareSSPin }; //Ethernet and SD on Hardware SS

void ProcessCache(boolean p_force);
void AppendCacheNode(const byte p_type, const String &p_data);

void HTMLParmInsert(const int &p_name, const String &p_value);
void HTMLParmDelete(const int &p_name);
String HTMLParmRead(const int &p_name);
void HTMLParmListDispose();

boolean CacheFileExists();

String CharRepeat(const char p_char, const int p_length);

String Message(int p_msg_num);
String GetSubFileChar(int p_hh);

const unsigned long C_OneSecond = 1000;    //one second


//
// END OF FILE
//
