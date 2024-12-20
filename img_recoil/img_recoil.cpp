


#include "img_recoil.h"
#include "../img_handler.h"
#include "../utils/utils.h"
#ifdef WITH_RECOIL

#include "../external/recoil/recoil-stdio.h"

#ifdef USE_CUSTOM_RECOIL_CHECK
char recoil_ext[][8]={"ABK","ACBM","DEEP","FLF","HAM","HAM6","HAM8","IFF","256","INFO","LBM","ILBM","DHR","DR","MP","BEAM","RGB8","RGBN","SHAM","DCT","DCTV","IFF","CM5","GFX","FLF","FNT","HGB","PPH","ODD","EVE","SCR","PAL","SGX","WIN","PAL","HGR","SPR","DHGR","DHR","3201","32K","GS","IIGS","PNT","SHR","PNT","SH3","3200","SHR","MAC","PNT","PNTG","256","AP2","4MI","4PL","4PM","A4R","ACS","AGP","AGS","ALL","AN2","AN4","AN5","AP3","APV","DGI","DGP","ESC","ILC","PZM","APA","APC","PLM","APL","APP","APS","ART","ART","ART","ASC","BG9","G09","BGP","BKG","CCI","CHR","CIN","CPI","CPR","CUT","DIN","DIT","DLM","DRG","F80","FGE","FN2","FNT","FWA","G10","G11","G2F","G9S","SFD","GED","GHG","GR0","ASC","SCR","GR1","GR2","GR3","GR7","GR8","GR9","GR9P","HCI","HR2","HCM","HIP","HPM","HPS","HR","ICE","ICN","IGE","ILD","ILS","IMN","ING","INP","INS","INT","IP2","IPC","IR2","IRG","IST","JGP","KPR","KSS","LDM","LEO","LUM","COL","KFX","MAP","MAP","MAX","MBG","MCH","MCP","MCPP","MCS","MGA","MGP","MIC","MIC","MIC","COL","MIC","PMG","RP","RP.INI","MIS","MPL","MSL","NLQ","ODF","PGR","PI8","PI9","PIC","PIC","PIC","PIC","PIX","PLA","PLS","PMD","PSF","RAP","RAW","RGB","RIP","RM0","RM1","RM2","RM3","RM4","RYS","SG3","SGE","SHC","SHP","SHP","SIF","SKP","SPC","SPR","SPR","SXS","TIP","TL4","TX0","TXE","TXS","VSC","G2F","VZI","WND","XLP","ZM4","DAP","G2F","PGC","PGF","ART","ART","ART","BIL","BL1","BL2","BL3","BLD","BP1","BP2","BP4","C01","C02","C04","BRU","CA1","CA2","CA3","CE1","CE2","CE3","CEL","CMP","COL","CP3","CPT","CPT","HBL","CRG","DA4","DOO","ART","DU1","DUO","DU2","EZA","FLF","FNT","FNT","FUL","GFB","GRX","HPK","HRM","IC1","IC2","IC3","ICN","IFF","IM","IMG","KID","LPK","MPK","MPP","MUR","PAL","NEO","NEO","NEO","RST","OBJ","OBJ","P3C","PA3","PAC","PBX","PC1","PC2","PC3","PCI","PCS","PG0","PG1","PG1","PG2","PG2","PG3","PI1","PI2","PI3","SUH","PIC","PL4","PPP","PSC","RGB","RGH","CL0","SC0","CL1","SC1","CL2","SC2","SD0","SD1","SD2","SPC","SPS","SPU","SPU","SPX","SRT","SSB","TN1","TN4","TN2","TN5","TN3","TN6","TNY","XIMG","PI4","PI5","PI6","B&W","B_W","BP6","BP8","C06","C08","C16","C24","C32","DC1","DEL","DG1","DPH","ESM","FTC","FUN","GOD","HIR","IB3","IBI","IIM","PI4","PI7","PI9","PIX","PNT","TPI","RAG","RAGC","RAW","RWH","RWL","TCP","TG1","TIMG","TRE","TRP","TRP","TRU","XGA","BB0","BB1","BB2","BB4","BB5","BBG","FLF","BP","FLF","MG","PIC0","PIC1","4BT","64C","A","A64","WIG","AAS","ART","AFL","AMI","BDP","BFL","BFLI","BML","FLI","FLG","BS","CDU","CFLI","CGX","CHE","CLE","CLP","CTM","CWG","DD","DDP","DOL","BED","DRL","DLP","DRZ","DRP","ECI","ECP","EMC","ESH","FBI","FCP","FPT","FD2","FED","FFL","FFLI","FLF","FLI","FLM","FP","FUN","FP2","FPR","G","GB","GCD","MON","GG","GR","CS","GUN","IFL","HBM","HIR","HPI","FGS","HCB","HED","HET","HFC","HFD","HIM","HLE","HLF","HIE","HPC","IHE","ILE","IPH","GIG","HPI","HRE","IPT","LRE","ISH","ISH","ISM","JJ","GIG","KLA","KOA","LP3","MCI","MIL","MLE","MUF","MUI","MUP","MWI","MWIN","NUF","NUP","OCP","MPI","MPIC","P64","FLY","PBOT","PDR","PET","PET","PG","PI","BPL","PMG","PP","PPP","RP","RPH","RPM","RPO","GIH","SAR","SCR","COL","SH1","SH2","SHE","SHE","SHF","SHI","SHP","SHS","SHX","SIF","SPD","UFL","UIF","VHI","VIC","VID","XFL","ZOM","ZS","P4I","P4I","P4I","BM","VBM","BRUS","PICT","IP","PIC","BKS","SCR","PI","PIC","GRB","GRO","SC2","GRP","SC3","CMP","PL5","FNT","GL5","SH5","PL5","GL6","SH6","PL6","GL7","SH7","PL7","GL8","SH8","MAG","MAX","MIF","MIG","PCT","PI","PIC","SC4","SC5","GE5","SC5","S15","SC6","SC6","S16","SC7","GE7","SC7","S17","SC8","GE8","PIC","SC8","S18","SR5","PL5","SR6","PL6","SR7","PL7","SR8","SRI","PL7","STP","GLA","GLB","SHA","SHB","PLA","GLC","GLS","SHC","SCA","SCB","SCA","S1A","SCC","SRS","YJK","SCC","S1C","G9B","MAG","IMG","IMG","KTY","MAG","PI","KT4","MAG","PIC","ARV","EBD","MAG","MKI","ML1","MX1","NL3","PI","Q4","ZIM","CHS","HIR","HRS","EPA","HS2","FLF","FNT","MSP","TIM","PIC","ICN","LCE","SS1","SS2","SS3","SS4","SCS4","SSX","MAG","PIC","PNT","HRG","SCR","SCR","HR","RLE","SHR","CLP","GRF","MAX","P41","PIX","P11","SPR","P","RAW","ZP1","3","ATR","BMC4","BSC","BSP","CH$","CH4","CH6","CH8","CHX","FLF","HLR","IFL","IMG","MC","MG1","MG2","MG4","MG8","MLT","RGB","SCR","SEV","STL","ZXP","ZXS","GRF","SCR","SXG","NXI"};
#endif

void st_Win_Print_Recoil(int16_t this_win_handle);
void _st_Read_Recoil(int16_t this_win_handle, boolean file_process);

#ifdef USE_CUSTOM_RECOIL_CHECK
bool st_Check_Recoil_Ext(const char* this_ext){
    for (size_t i = 0; i < sizeof(recoil_ext) / sizeof(recoil_ext[0]); i++)
    {
        if (check_ext(this_ext, recoil_ext[i])){
            return true;
        }
    }
    return false;
}
#endif

void st_Init_Recoil(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_Recoil;

    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Print_Recoil(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_Recoil(this_win_handle, this_win->prefers_file_instead_mem);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_Recoil(int16_t this_win_handle, boolean file_process){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){

    const char *input_file = this_win->wi_data->path;

	RECOIL *recoil = RECOILStdio_New();
	if (recoil == NULL) {
        sprintf(alert_message, "Out Of Mem Error");
        st_form_alert(FORM_EXCLAM, alert_message);
		return;
	}

	if (!RECOILStdio_Load(recoil, input_file)) {
        sprintf(alert_message, "%s\nFile decoding error", input_file);
        st_form_alert(FORM_EXCLAM, alert_message);
		return;
	}

		u_int16_t width = RECOIL_GetWidth(recoil) ;
		u_int16_t height = RECOIL_GetHeight(recoil);
        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
		mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);

        u_int32_t total_pixels = width * height;

		u_int8_t* src_RGB = (u_int8_t*)RECOIL_GetPixels(recoil);

        u_int32_t* dst_ARGB = (u_int32_t*)destination_buffer;

        for(int y = 0; y < height; y++){
            for(int x = 0; x < width; x++){
                int i = (MFDB_STRIDE(width) * y) + x;
                // int j = ((width * y) + x) * 3;
                dst_ARGB[i] = (*src_RGB++) << 24 | (*src_RGB++) << 16 | (*src_RGB++) << 8 | *src_RGB++ ;
            }                
        }

        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;
        RECOIL_Delete(recoil);	
	}
}

#endif