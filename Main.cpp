#include "Main.hpp"
#include "Header.hpp"
#include <limits>

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

    uint32_t lastsz = 0;
    uint32_t szcounter = 0;
    uint32_t maxsz = 0, maxsznum = 0;
    uint32_t minsz = std::numeric_limits<uint32_t>::max(), minsznum = 0;

    uint32_t timer;
    double sec;
    bool first = true;
    bool first2 = true;
    double timerval = 0;
    double timercounter = 0;
    double lasttimer = 0;
    double mintimer = std::numeric_limits<double>::max(); uint32_t mintimernum = 0;
    double maxtimer = 0; uint32_t maxtimernum = 0;
    double starttimer = 0;
    double endtimer = 0;
    if(argc != 2 && argc != 3){
        cout << "JPEG file expander\nUsage:\t" << argv[0] << " <infile> " << endl;
        return 0;
    }
    FILE* in = fopen(argv[1],"rb");
    if(!in){
        cout << "Can't open infile" << endl;
        return -1;
    }

    char str[64];
    std::time_t t = time(nullptr);
    strftime(str, 64, "mkdir .\\out\\images-%d_%m_%Y-%H_%M_%S" ,localtime(&t));
    system(str);
    strftime(str, 64, ".\\out\\images-%d_%m_%Y-%H_%M_%S\\" ,localtime(&t));

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
            lastsz = 0;
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
            lastsz += szSOI;
            uint8_t q;
            uint16_t w,h;
            fread(&q,1,1,in);
            fread(&h,2,1,in);
            fread(&w,2,1,in);
            lastsz += 5;

            fwrite(DQT_Y,szDQT_Y,1,out);
            fwrite(&QTABLES_Y[szQTABLE*q], szQTABLE, 1, out);
            lastsz += szQTABLE + szDQT_Y;

            fwrite(DQT_UV,szDQT_UV,1,out);
            fwrite(&QTABLES_UV[szQTABLE*q], szQTABLE, 1, out);
            lastsz += szQTABLE + szDQT_UV;

            fwrite(SOF_A,szSOF_A,1,out);
            fwrite(&h,2,1,out);
            fwrite(&w,2,1,out);
            fwrite(SOF_B,szSOF_B,1,out);
            lastsz += szSOF_A + 2 + 2 + szSOF_B;

            fwrite(DHT_Y_DC,szDHT_Y_DC,1,out);
            fwrite(Y_DC_TABLE, szY_DC_TABLE,1,out);
            lastsz += szDHT_Y_DC + szY_DC_TABLE;

            fwrite(DHT_Y_AC,szDHT_Y_AC,1,out);
            fwrite(Y_AC_TABLE,szY_AC_TABLE,1,out);
            lastsz += szDHT_Y_AC + szY_AC_TABLE;

            fwrite(DHT_UV_DC,szDHT_UV_DC,1,out);
            fwrite(UV_DC_TABLE, szUV_DC_TABLE, 1, out);
            lastsz += szDHT_UV_DC + szUV_DC_TABLE;

            fwrite(DHT_UV_AC,szDHT_UV_AC,1,out);
            fwrite(UV_AC_TABLE,szUV_AC_TABLE,1,out);
            lastsz += szDHT_UV_AC + szUV_AC_TABLE;

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
                uint8_t mtimer[4];
                fread(&mtimer, 4, 1, in);
                timer = mtimer[3] | (mtimer[2] << 8) | (mtimer[1] << 16) | (mtimer[0] << 24);
                sec = timer/32768.0;

                if(first){
                    first = false;
                    timerval = sec;
                    lasttimer = 0;
                    starttimer = sec;
                    timercounter = 0;
                } else {
                    if(sec > timerval) lasttimer = sec - timerval;
                    else if(sec < lasttimer) lasttimer = sec;
                    timerval = sec;
                    if(lasttimer > maxtimer) maxtimer = lasttimer, maxtimernum = counter;
                    if(lasttimer < mintimer) mintimer = lasttimer, mintimernum = counter;
                    timercounter+= lasttimer;
                }

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
                lastsz += szEXIF;
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
                    lastsz++;
                    tmp = q;
                }
                fwrite(&tmp, 1, 1, out);
                lastsz++;
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
                    lastsz++;
                    tmp = q;
                }
                fwrite(&tmp, 1, 1, out);
                lastsz++;
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
                    lastsz++;
                    tmp = q;
                }
                fwrite(&tmp, 1, 1, out);
                lastsz++;
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
            lastsz+= szEOI;
            fclose(out);

            if(first2) first2 = false;
            else{
                szcounter += lastsz;
                if(lastsz >maxsz ) maxsz = lastsz, maxsznum = counter;
                if(lastsz < minsz ) minsz = lastsz, minsznum = counter;
            }
    }
final:
    endtimer = sec;
    fclose(in);
    char sname[64];
    sprintf(sname, "%s%s", str, "stat.log");
    FILE* sout = fopen(sname, "w");
    fprintf(sout, "%d files:\n\tMax size - %d bytes (#%d)\n\tMin size - %d bytes (#%d)\n\tAvg size - %d bytes\n%.3f seconds:\n\t%.3f seconds max (#%d)\n\t%.3f seconds min (#%d)\nStart - %.3f seconds ( real 0.000)\nStop - %.3f seconds (real %.3f)\n",
                    counter,                  maxsz,    maxsznum,          minsz,    minsznum,(szcounter)/counter, timercounter, maxtimer, maxtimernum, mintimer,       mintimernum,  starttimer,                     endtimer,        timercounter);
    fclose(sout);
    cout << "Expanded successfully " << counter << " files." << endl;

    return 0;
}
