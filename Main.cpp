#include "Main.hpp"
#include "Header.hpp"

bool writeStart(uint8_t c, FILE* out){
    switch(c){
        case 0xb0: fwrite(SOS_Y,szSOS_Y,1,out);
            break;
        case 0xb1: fwrite(SOS_U,szSOS_U,1,out);
            break;
        case 0xb2: fwrite(SOS_V,szSOS_V,1,out);
            break;
        default: return false;
    }
    return true;
}

int main(int argc, char** argv){
    uint32_t counter = 0;
    if(argc != 2){
        cout << "JPEG file expander\nUsage:\t" << argv[0] << " <infile>" << endl;
        return 0;
    }
    FILE* in = fopen(argv[1],"rb");
    if(!in){
        cout << "Can't open infile" << endl;
        return -1;
    }

    char str[64];
    std::time_t t = time(nullptr);
    strftime(str, 64, "mkdir .\\images-%d_%m_%Y-%H_%M_%S" ,localtime(&t));
    system(str);
    strftime(str, 64, ".\\images-%d_%m_%Y-%H_%M_%S\\" ,localtime(&t));

    while(!feof(in)){
            uint8_t sof[2] = {0, 0};
            while(!feof(in)){
                sof[1] = sof[0];
                fread(sof, 1, 1, in);
                if(*((uint16_t *)sof) == 0xFFD8) goto sof_found;
            }
            cout << "Can't find Start of File" << endl;
            //return -2;
            goto final;

        sof_found:
            counter++;
            char filename[64];
            sprintf(filename,"%s%d.jpg", str,counter);
            FILE* out = fopen(filename, "wb");
            if(!out){
                cout << "Can't open outfile" << endl;
                goto final;
            }
            cout << filename << endl;
            fwrite(SOI,szSOI,1, out);
            uint8_t q;
            uint16_t w,h;
            fread(&q,1,1,in);
            fread(&h,2,1,in);
            fread(&w,2,1,in);

            fwrite(DQT_Y,szDQT_Y,1,out);
            fwrite(&QTABLES_Y[szQTABLE*q], szQTABLE, 1, out);

            fwrite(DQT_UV,szDQT_UV,1,out);
            fwrite(&QTABLES_UV[szQTABLE*q], szQTABLE, 1, out);

            fwrite(SOF_A,szSOF_A,1,out);
            fwrite(&h,2,1,out);
            fwrite(&w,2,1,out);
            fwrite(SOF_B,szSOF_B,1,out);

            fwrite(DHT_Y_DC,szDHT_Y_DC,1,out);
            fwrite(Y_DC_TABLE, szY_DC_TABLE,1,out);

            fwrite(DHT_Y_AC,szDHT_Y_AC,1,out);
            fwrite(Y_AC_TABLE,szY_AC_TABLE,1,out);

            fwrite(DHT_UV_DC,szDHT_UV_DC,1,out);
            fwrite(UV_DC_TABLE, szUV_DC_TABLE, 1, out);

            fwrite(DHT_UV_AC,szDHT_UV_AC,1,out);
            fwrite(UV_AC_TABLE,szUV_AC_TABLE,1,out);

            while(!feof(in)){
                uint8_t tmp;
                fread(&tmp,1,1,in);
                if(tmp == 0xFF){
                    fread(&q,1,1,in);
                    if(q) goto pred_found;
                }
            }
            cout << "Can't find PT/T/C1 component" << endl;
            goto loopstart;

        pred_found:
            if(q != 0xA0){
                goto time_found;
            }
            {
                char filename2[64];
                sprintf(filename2,"%s%d.csv", str,counter);
                FILE* pout = fopen(filename2, "w");
                int cn = 0;
                int bw = (w&255) << 8 | w >> 8;
                while(!feof(in)){
                    uint8_t tmp;
                    fread(&tmp,1,1,in);
                    if(tmp == 0xFF){
                        fread(&q,1,1,in);
                        if(q){
                            fclose(pout);
                            goto c1_found;
                        }
                    }
                    fprintf(pout, "%d;", tmp);
                    cn++;
                    if(cn >= bw/40) fprintf(pout, "\n"), cn = 0;
                }
            }
        time_found:
            if(q != 0xA1){
                goto c1_found;
            }
            {
                char exifheader[szEXIF];
                memcpy(exifheader, EXIF, szEXIF);
                char datetime[EXIFDATETIME_len];
                char subsec[EXIFSUBSEC_len];
                memset(datetime, 0, EXIFDATETIME_len);
                memset(subsec, 0, EXIFSUBSEC_len);
                uint32_t timer = 0;
                uint8_t mtimer[4];
                fread(&mtimer, 4, 1, in);
                timer = mtimer[3] | (mtimer[2] << 8) | (mtimer[1] << 16) | (mtimer[0] << 24);


                double sec = timer/32768.0;
                unsigned int totalmillisecs = sec*1000.0;
                unsigned int totalsecs = totalmillisecs/1000;
                unsigned int totalmins = totalsecs/60;
                unsigned int totalhours = totalmins/60;
                unsigned int totaldays = totalhours/24;
                unsigned int days = totaldays;
                unsigned int hours = totalhours%24;
                unsigned int mins = totalmins%60;
                unsigned int secs = totalsecs%60;
                unsigned int millisecs = totalmillisecs%1000;

                uint8_t id[11];
                uint8_t exposurefrac[8];
                fread(exposurefrac, 8, 1, in);
                fread(id, 11, 1, in);
                char exifid[EXIFID_len];

                snprintf(exifid, EXIFID_len, "%03d-%020lld-%05d", id[0], (uint64_t)((uint64_t)id[1] + ((uint64_t)id[2] << 8) + ((uint64_t)id[3] << 16) + ((uint64_t)id[4] << 24) + ((uint64_t)id[5] << 32) + ((uint64_t)id[6] << 40) +((uint64_t)id[7] << 48) + ((uint64_t)id[8] << 56)), (uint16_t)((uint16_t)id[9] + ((uint16_t)id[10] << 8)));
                snprintf(datetime,EXIFDATETIME_len,"%04d:%02d:%02d %02d:%02d:%02d", 1, 1, days+1, hours, mins, secs);
                snprintf(subsec, EXIFSUBSEC_len, "%03d", millisecs);
                memcpy(exifheader+EXIFSUBSEC_pos, subsec, EXIFSUBSEC_len);
                memcpy(exifheader+EXIFDATETIME_pos, datetime, EXIFDATETIME_len);
                memcpy(exifheader+EXIFID_pos, exifid, EXIFID_len);
                memcpy(exifheader+EXIFEXP_pos, exposurefrac, EXIFEXP_len);
                fwrite(exifheader, szEXIF, 1, out);
            }
            while(!feof(in)){
                uint8_t tmp;
                fread(&tmp, 1, 1, in);
                if(tmp == 0xFF){
                    fread(&q,1,1,in);
                    if(q) goto c1_found;
                }
            }
            cout << "Can't find component 1 marker" << endl;
            goto loopstart;

        c1_found:
            if(!writeStart(q,out)){
                cout << "Bad component #1 marker" << endl;
                goto loopstart;
            }
            while(!feof(in)){
                uint8_t tmp;
                fread(&tmp, 1, 1, in);
                if(tmp == 0xFF){
                    fread(&q,1,1,in);
                    if(q) goto c2_found;
                    fwrite(&tmp, 1, 1, out);
                    tmp = q;
                }
                fwrite(&tmp, 1, 1, out);
            }

        c2_found:
            if(!writeStart(q,out)){
                cout << "Bad component #2 marker" << endl;
                goto loopstart;
            }
            while(!feof(in)){
                uint8_t tmp;
                fread(&tmp, 1, 1, in);
                if(tmp == 0xFF){
                    fread(&q,1,1,in);
                    if(q) goto c3_found;
                    fwrite(&tmp, 1, 1, out);
                    tmp = q;
                }
                fwrite(&tmp, 1, 1, out);
            }

        c3_found:
            if(!writeStart(q,out)){
                cout << "Bad component #3 marker" << endl;
                goto loopstart;
            }
            while(!feof(in)){
                uint8_t tmp;
                fread(&tmp, 1, 1, in);
                if(tmp == 0xFF){
                    fread(&q,1,1,in);
                    if(q) goto end_found;
                    fwrite(&tmp, 1, 1, out);
                    tmp = q;
                }
                fwrite(&tmp, 1, 1, out);
            }

        end_found:
            if(q == 0xD8){
                cout << "End marker not found; Start marker found instead." << endl;
                fseek(in,-2,SEEK_CUR);
            } else if(q != 0xD9){
                cout << "Bad end marker" << endl;
                goto loopstart;
            }
loopstart:
            fwrite(EOI,szEOI,1,out);
            fclose(out);
    }
final:
    fclose(in);
    cout << "Expanded successfully " << counter << " files." << endl;

    return 0;
}
