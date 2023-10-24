#include "file.h"

int fileno __P ((FILE *__stream));

boolean get_stat_file(struct_metadata *my_wi_data);
const char *get_filename_ext(const char *filename);

boolean open_file(struct_window *this_win, const char *my_path){
	if(this_win->wi_data != NULL){
		if(this_win->wi_data->path == NULL){
			this_win->wi_data->path = (char*)mem_alloc(strlen(my_path) + 1);
			strcpy((char*)this_win->wi_data->path, my_path);
		}
		this_win->wi_data->file_lock = fopen(this_win->wi_data->path,READ_ONLY_BINARY);
		if(!this_win->wi_data->file_lock){
			sprintf(alert_message, "Open: Error reading file\n%s", this_win->wi_data->path);
			st_form_alert(FORM_STOP, alert_message);
			return false;
		}
		this_win->prefers_file_instead_mem = DO_WE_USE_FILE;
	}else{
		sprintf(alert_message, "Window Data Struct Error");
        st_form_alert(FORM_STOP, alert_message);
		return false;
	}
	if (this_win->wi_data->file_lock != NULL){
		get_stat_file(this_win->wi_data);
		this_win->wi_data->extension = (char*)get_filename_ext(this_win->wi_data->path);
		fclose(this_win->wi_data->file_lock);
	}
	return true;
}

boolean get_stat_file(struct_metadata *my_wi_data){
	return (fstat(fileno(my_wi_data->file_lock), &my_wi_data->STAT_FILE) == 0 && S_ISREG(my_wi_data->STAT_FILE.st_mode)) ? TRUE : FALSE;
}

const char *get_filename_ext(const char *filename){
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

boolean file_to_memory(struct_window *this_win){
	this_win->wi_data->file_lock = fopen(this_win->wi_data->path,READ_ONLY_BINARY);
	if(!this_win->wi_data->file_lock){		
		sprintf(alert_message, "File2Mem: Error reading file\n%s", this_win->wi_data->path);
        st_form_alert(FORM_STOP, alert_message);
		return false;
	}
	this_win->wi_data->original_buffer = (int8_t *)mem_alloc(this_win->wi_data->STAT_FILE.st_size);
	if(this_win->wi_data->original_buffer == NULL){
		sprintf(alert_message, "Mem Alloc error\n%lu.o", this_win->wi_data->STAT_FILE.st_size);
        st_form_alert(FORM_STOP, alert_message);
		fclose(this_win->wi_data->file_lock);
		return false;
	}
	fread(this_win->wi_data->original_buffer, sizeof(char), this_win->wi_data->STAT_FILE.st_size, this_win->wi_data->file_lock);
	fclose(this_win->wi_data->file_lock);
	return true;
}

boolean mfdb_to_file(MFDB *this_mfdb, char* final_path){
	FILE *fp;
	u_int32_t size;
	if((fp = fopen(final_path,WRITE_BINARY)) == NULL){
		sprintf(alert_message, "Unable to open %s for writing", final_path);
		st_form_alert(FORM_STOP, alert_message);
		return false;
	} else {
		if(this_mfdb->fd_nplanes < 8){
			size = (MFDB_STRIDE(this_mfdb->fd_w) * this_mfdb->fd_h) / (8 / this_mfdb->fd_nplanes);
		} else {
			size = MFDB_STRIDE(this_mfdb->fd_w) * this_mfdb->fd_h * (this_mfdb->fd_nplanes >> 3);
		}
		int16_t err = fwrite(this_mfdb->fd_addr,  size, 1, fp);
		if( err != 1) {
			sprintf(alert_message, "Write error\nerr %d\n%s", err, final_path);
			st_form_alert(FORM_STOP, alert_message);			
			return false;
		}
		fclose(fp);
		return true;		
	}
}

int16_t file_selector(char *final_path, char* title, char *file_selected){
	int16_t current_drive, button_value, i;
	
	char this_current_path[256] = {'\0'};
	current_drive = st_Dgetdrv();
	st_Dgetpath(this_current_path,0);
	final_path[0] = current_drive + 65;
	final_path[1] = ':';
	strcat(final_path,this_current_path);
	strcat(final_path,"\\*.*");

	// wind_update(BEG_MCTRL);
	fsel_exinput(final_path, file_selected,&button_value, title);
	// wind_update(END_MCTRL);
	
	if(button_value != FALSE){
		i = strlen(final_path);
		while(i && final_path[i - 1] != '\\'){
			i--;
		}
		final_path[i] = '\0';
		strcat(final_path,file_selected);
		return TRUE;
	} else {
		return FALSE;
	}
}
