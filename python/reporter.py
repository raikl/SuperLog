# -*- coding: utf-8 -*-
import datetime
from datetime import time
import dateutil.relativedelta
import os
import xlwt
import win32com.client as win32
import pythoncom



def timeRound(a):
    return time(a.hour, a.minute, 0)

def obtenerColnumaStr(n):
    string = ""
    while n > 0:
        n, remainder = divmod(n - 1, 26)
        string = chr(65 + remainder) + string
    return string

class Reporter:
    def leerCargaRutas(self, archivo):
        cargaRutas = {}
        origenes = {}
        arch = open(archivo,"r") 
        for l in arch.readlines():
            sl = l.split(";")
            ssi = ""
            if sl[7] == "1":
                ssi = "si"
            cargaRutas[tuple(sl[0:6])] = (float(sl[6]), ssi, float(sl[8]), sl[9], sl[10][2:])
            if not sl[1] in origenes.keys():
                origenes[sl[1]] = {}
            if not sl[4] in origenes[sl[1]].keys():
                origenes[sl[1]][sl[4]] = {}
            origenes[sl[1]][sl[4]][sl[5]] = 0
        arch.close()
        return cargaRutas, origenes
        
    def leerCuadratura(self, archivo):
        cuadratura = {}
        destinos = {}
        arch = open(archivo,"r") 
        for l in arch.readlines():
            sl = l.split(";")
            if not sl[0] in destinos.keys():
                destinos[sl[0]] = 0
            if not self.materiales[int(sl[1])].estado in cuadratura.keys():
                cuadratura[self.materiales[int(sl[1])].estado] = {}
            if not self.materiales[int(sl[1])].sector in cuadratura[self.materiales[int(sl[1])].estado].keys():
                cuadratura[self.materiales[int(sl[1])].estado][self.materiales[int(sl[1])].sector] = {}
            if not int(sl[1]) in cuadratura[self.materiales[int(sl[1])].estado][self.materiales[int(sl[1])].sector].keys():
                cuadratura[self.materiales[int(sl[1])].estado][self.materiales[int(sl[1])].sector][int(sl[1])] = (self.materiales[int(sl[1])].descripcion, [])
            cuadratura[self.materiales[int(sl[1])].estado][self.materiales[int(sl[1])].sector][int(sl[1])][1].append((sl[0], sl[2]))
        arch.close()
        return cuadratura, destinos
    
    
    def grabarCuadratura(self):
        print("-Grabando Cuadrarura-")
        fc = {"Refrigerado":('F', 'light_yellow', 'Fresco'), "Congelado":('C', 'pale_blue', 'Congelado')}
        cuadratura, destinos = self.leerCuadratura(self.pathTmpLogistica + "/Cuadratura.txt")
        wb = xlwt.Workbook()
        ws = wb.add_sheet("Cuadratura")        
        cnt = 3
        sts = xlwt.easyxf('font: colour black; pattern: pattern solid; borders: left thin, right thin, top thin, bottom thin;')
        stsb = xlwt.easyxf('font: bold true, colour black; pattern: pattern solid; borders: left thin, right thin, top thin, bottom thin;')
        stsr = xlwt.easyxf('font: bold true, colour black; pattern: pattern solid; align: rotation 90; borders: left thin, right thin, top thin, bottom thin; align: vert centre, horiz centre;')
        sts.pattern.pattern_fore_colour = xlwt.Style.colour_map["gray25"]
        stsb.pattern.pattern_fore_colour = xlwt.Style.colour_map["gray25"]
        stsr.pattern.pattern_fore_colour = xlwt.Style.colour_map["gray25"]
        stn = xlwt.easyxf('font: bold true, colour white; align: rotation 90; pattern: pattern solid; borders: left thin, right thin, top thin, bottom thin; align: vert centre, horiz centre;')
        stn.pattern.pattern_fore_colour = xlwt.Style.colour_map["light_blue"]
        ws.write_merge(1, 2, 2, 2, "Und.", sts)
        ws.write_merge(3, 5, 0, 0, "Cuadratura", stsb)
        ws.write(3, 1, "Total", stsb)
        ws.write(3, 2, "Pl", stsb)
        ccnt = 1     
        pInicio = len(cuadratura) + 4
        for e in destinos.keys():
            vr = e
            if not e in self.nodos.keys():
                vr = int(e)
            if vr == 60472:
                vr = "P" + str(int(vr))
            ws.write(2, cnt, self.nodos[vr][2], stn)
            ws.write(1, cnt, self.nodos[vr][0][0], sts)
            destinos[e] = cnt
            ws.write(3, cnt, xlwt.Formula("SUM(" + obtenerColnumaStr(cnt + 1) + "5:" + obtenerColnumaStr(cnt + 1) + "6)"))
            cnt += 1        
        pi = 4 + len(cuadratura.keys())
        for e in cuadratura.keys():
            ws.write(3 + ccnt, 1, "Total " + fc[e][2] + " Camión", stsb)
            ws.write(3 + ccnt, 2, "Pl", stsb)
            pi += len(cuadratura[e]) + 1
            for d in destinos.keys():
                ws.write(3 + ccnt, destinos[d], xlwt.Formula(obtenerColnumaStr(destinos[d] + 1) + str(pi)))
            ccnt += 1            
        pInicio += len(cuadratura[e])
        for e in cuadratura.keys():
            ws.write_merge(3 + ccnt, 3 + ccnt + len(cuadratura[e]), 0, 0, fc[e][2], stsr)
            for s in cuadratura[e].keys():
                ws.write(3 + ccnt, 1, "Total " + s, sts)
                ws.write(3 + ccnt, 2, "Pl", sts)
                pInicio += len(cuadratura[e][s])    
                ccnt += 1
            pInicio -= 1
            ws.write(3 + ccnt, 1, "Total " + fc[e][2] + " por Cliente", stsb)
            ws.write(3 + ccnt, 2, "Pl", stsb)
            for o in destinos.keys():
                ws.write(3 + ccnt, destinos[o], xlwt.Formula("SUM(" + obtenerColnumaStr(destinos[o] + 1) + str(4 + ccnt - len(cuadratura[e].keys())) +":" + obtenerColnumaStr(destinos[o] + 1) + str(3 + ccnt) + ")"))
            ccnt += 1
        ws.write(3 + ccnt, 0, "Código", stsb)
        ws.write(3 + ccnt, 1, "Producto", stsb)
        cnt = ccnt + 4
        anchoCol = 0
        tcnt = 1 + len(cuadratura.keys())
        for e in cuadratura.keys():
            st = xlwt.easyxf('pattern: pattern solid;')            
            st.pattern.pattern_fore_colour = xlwt.Style.colour_map[fc[e][1]]        
            stb = xlwt.easyxf('font: bold true, colour black; pattern: pattern solid;')            
            stb.pattern.pattern_fore_colour = xlwt.Style.colour_map[fc[e][1]]        
            totSectorDestino = {}
            for s in cuadratura[e].keys():
                ws.write(cnt, 1, s, stb)
                cnt += 1
                totSectorDestino[s] = {}
                for o in destinos.keys():
                    totSectorDestino[s][o] = 0
                for p in cuadratura[e][s].keys():
                    ws.write(cnt, 0, p, st)
                    ws.write(cnt, 1, cuadratura[e][s][p][0], st)
                    anchoCol = max(len(cuadratura[e][s][p][0]), anchoCol)
                    ws.write(cnt, 2, "E", st)
                    for t in cuadratura[e][s][p][1]:
                        ws.write(cnt, destinos[t[0]], float(t[1]), st)
                        totSectorDestino[s][t[0]] += (float(t[1])/self.materiales[p].cajasPallet)
                    cnt += 1
                for o in destinos.keys():
                    ws.write(3 + tcnt, destinos[o], totSectorDestino[s][o])
                tcnt +=1
            tcnt += 1
        ws.col(1).width = anchoCol * 256
        wb.save(self.archivoCuadratura)
    
    def grabarRutas(self):
        print("-Grabando Rutas-")
        fc = {"Refrigerado":('F', 'light_yellow'), "Congelado":('C', 'pale_blue')}
        cargaRutas, origenes = self.leerCargaRutas(self.pathTmpLogistica + "/Rutas.txt")
        wb = xlwt.Workbook()
        ws = wb.add_sheet("Rutas", cell_overwrite_ok=True)                
        sts = xlwt.easyxf('font: colour black; pattern: pattern solid; borders: left thin, right thin, top thin, bottom thin;')
        stsNum = xlwt.easyxf('font: colour black;', num_format_str = '#,###')
        stsDateTime = xlwt.easyxf('font: colour black;', num_format_str='DD/MM/YYYY HH:mm')
        sts.pattern.pattern_fore_colour = xlwt.Style.colour_map["grey25"]
        
        idx = open(self.pathTmpLogistica + "/capacidades.txt", "r") 
        lcnt = 0
        tiemposCamino = {}
        for lidx in idx.readlines():
            csl = lidx.split(";")[0:-1]
            if lcnt > 0:
                tiemposCamino[csl[0], csl[3]] = [int(csl[5]), int(csl[6])]
            lcnt = +1
        
        ws.write(2, 0, "Número", sts)
        ws.write(3, 0, "Camión", sts)
        ws.write(2, 1, "Rut", sts)
        ws.write(3, 1, "Transp.", sts)
        ws.write(2, 2, "Fecha-Hora", sts)
        ws.write(3, 2, "Inicio", sts)
        ws.write(2, 3, "Fecha-Hora", sts)
        ws.write(3, 3, "Término", sts)
        ws.write(2, 4, "Tipo", sts)
        ws.write(3, 4, "Viaje", sts)
        ws.write(2, 5, "Costo", sts)
        ws.write(3, 5, "Ruta", sts)
        ws.write(2, 6, "Camión", sts)
        ws.write(3, 6, "Suministrador", sts)
        ws.write(2, 7, "Códigos", sts)
        ws.write(3, 7, "Ruta", sts)
        ws.write(2, 8, "Descripción", sts)
        ws.write(3, 8, "Ruta", sts)
        ws.write(2, 9, "Tipo", sts)
        ws.write(3, 9, "Destino", sts)
        ws.write(2, 10, "Tipo", sts)
        ws.write(3, 10, "Camión", sts)
        ws.write(2, 11, "", sts)
        ws.write(3, 11, "Separador", sts)
        ws.write(2, 12, "Total", sts)
        ws.write(3, 12, "Pallets", sts)
        cntCol = 9 + 4
        for e in origenes.keys():
            ws.write(0, cntCol, self.nodos[e][2])
            for s in origenes[e].keys():
                for t in origenes[e][s].keys():
                    ws.write(2, cntCol, self.nodos[e][3], sts)
                    st = xlwt.easyxf('pattern: pattern solid; borders: left thin, right thin, top thin, bottom thin;')
                    st.pattern.pattern_fore_colour = xlwt.Style.colour_map[fc[t][1]]
                    ws.write(1, cntCol, xlwt.Formula("SUM(" + obtenerColnumaStr(cntCol + 1) +"5:" + obtenerColnumaStr(cntCol + 1) + str(len(cargaRutas) + 10) + ")"))
                    ws.write(3, cntCol, s[0:3] + " " + fc[t][0], st)
                    origenes[e][s][t] = cntCol
                    cntCol += 1
        cnt = 4        
        eAnt = list(cargaRutas.keys())[0]
        for e in cargaRutas.keys():
            if e[0] != eAnt[0] or e[2] != eAnt[2] or e[3] != eAnt[3]:
                cnt += 1
            spr = ""
            sTipo = ""
            epr = ""
            splr = e[0].split("-")
            for pr in splr:
                spr += self.nodos[pr][2] + "-"
                epr += self.varNodo[pr] + "-"
                sTipo = self.nodos[pr][0]
            spr = spr[:-1]
            epr = epr[:-1]
            try:
                ws.write(cnt, 1, self.nodoTransportista[splr[-1]])
            except:
                pass
            try:
                dtFecha = datetime.datetime.strptime(self.fecha.strftime('%Y%m%d'), '%Y%m%d')
                nFecha = dtFecha + datetime.timedelta(seconds=round(int(tiemposCamino[e[3], splr[0]][0])/60)*60)            
                ws.write(cnt, 2, nFecha, stsDateTime)
            except:
                pass
            try:
                dtFecha = datetime.datetime.strptime(self.fecha.strftime('%Y%m%d'), '%Y%m%d')
                nFecha = dtFecha + datetime.timedelta(seconds=round(int(tiemposCamino[e[3], splr[-1]][1])/60)*60)            
                ws.write(cnt, 3, nFecha, stsDateTime)
            except:
                pass
            ws.write(cnt, 6, cargaRutas[e][4])
            ws.write(cnt, 7, epr)
            ws.write(cnt, 8, spr[0:255])       
            ws.write(cnt, 9, sTipo)                   
            st = xlwt.easyxf('pattern: pattern solid;')
            st.pattern.pattern_fore_colour = xlwt.Style.colour_map[fc[e[5]][1]]                            
            ws.write(cnt, origenes[e[1]][e[4]][e[5]], float(cargaRutas[e][0]), st)            
            ws.write(cnt, 10, e[2].replace("P", " P"))
            ws.write(cnt, 11, cargaRutas[e][1])
            ws.write(cnt, 0, int(e[3]))
            ws.write(cnt, 5, cargaRutas[e][2], stsNum)
            ws.write(cnt, 4, cargaRutas[e][3])
            ws.write(cnt, 12, xlwt.Formula("SUM(" + obtenerColnumaStr(14) + str(cnt+1) + ":" + obtenerColnumaStr(cntCol) + str(cnt+1) + ")")) 
            eAnt = e
        wb.save(self.archivoRutas)
        
    def grabarIndicadores(self):
        print("-Grabando Indicadores-")
        sts = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;')
        stsNum = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;', num_format_str = '#,##0.000')
        stsb = xlwt.easyxf('font: bold true, colour black; borders: left thin, right thin, top thin, bottom thin;')
        wb = xlwt.Workbook()
        arch = open(self.pathTmpLogistica + "/catalogo-idx.txt","r") 
        cnt = 0
        cabeceraSinFormato = ["OUTCamiones", "INCamiones", "IDcamion", "#PtosCarga", "CodigoDestino", "CodigoNodo", "IDProducto", "CodigoNodoDestino"]
        for l in arch.readlines():
            sl = l.split(";")
            ws = wb.add_sheet(sl[1][:31].replace("Indicadores", "").replace("Indicador", ""))
            idx = open(self.pathTmpLogistica + "/" + sl[0],"r") 
            lcnt = 0
            cabecera = []
            for lidx in idx.readlines():
                csl = lidx.split(";")[0:-1]
                if lcnt == 0:
                    cabecera = csl
                ccnt = 0
                for cel in csl:
                    if (cnt == 0 and ccnt == 0) or (cnt != 0 and lcnt == 0):
                        cel = cel.replace("Ã³", "ó")
                        ws.write(lcnt, ccnt, cel, stsb)
                    else:
                        ssts = sts
                        if sl[0] == "IndicadorVisitaNodosPlanta.txt" and ccnt == 2:
                            spr = []
                            for pr in cel.split("-"):
                                spr.append(self.varNodo[pr])
                            cel = " - ".join(spr)
                        if cel in self.varNodo.keys():
                            cel = self.varNodo[cel]#.replace("T1632", "T163_2").replace("P60472", "60472")
                        try:
                            if cel != "0002" and cel != "60472":
                                cel = float(cel)  
                                if cabecera[ccnt] not in cabeceraSinFormato:
                                    ssts = stsNum
                        except:
                            pass
                        ws.write(lcnt, ccnt, cel, ssts)
                    ccnt += 1
                lcnt += 1
            cnt += 1

        stsDate = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;', num_format_str='DD/MM/YYYY')
        stsTime = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;', num_format_str='HH:mm')
        ws = wb.add_sheet("Utilización Capacidades")
        ws.write(0, 0, "Fecha de Programa", stsb)
        ws.write(0, 1, "Número Camión", stsb)
        ws.write(0, 2, "Tipo Camión", stsb)
        ws.write(0, 3, "Agente Servicios", stsb)
        ws.write(0, 4, "Tipo Viaje", stsb)
        ws.write(0, 5, "Tipo Nodo", stsb)
        ws.write(0, 6, "Nodo", stsb)
        ws.write(0, 7, "Nombre Nodo", stsb)
        ws.write(0, 8, "Ruta", stsb)
        ws.write(0, 9, "Fec.Lleg.", stsb)        
        ws.write(0, 10, "Hr.Lleg.", stsb)
        ws.write(0, 11, "Fec.Sal.", stsb)
        ws.write(0, 12, "Hor.Sal.", stsb)
        ws.write(0, 13, "Separador", stsb)
        ws.write(0, 14, "Compartido", stsb)
        ws.write(0, 15, "Sector", stsb)
        ws.write(0, 16, "Descripción Estado", stsb)
        ws.write(0, 17, "Pallets", stsb)
        ws.write(0, 18, "Kilos", stsb)
        ws.write(0, 19, "Inicio", stsb)        
        ws.write(0, 20, "Operación", stsb) 
        idx = open(self.pathTmpLogistica + "/capacidades.txt", "r") 
        lcnt = 0
        llaves = set()
        uAndenes = []
        for lidx in idx.readlines():
            csl = lidx.split(";")[0:-1]
            if lcnt > 0:
                uaLinea = []
                ws.write(lcnt, 0, self.fecha, stsDate)
                uaLinea.append(self.fecha)
                ws.write(lcnt, 1, float(csl[0]), sts)
                uaLinea.append(float(csl[0]))
                ws.write(lcnt, 2, csl[1].replace("P", " P"), sts)
                uaLinea.append(csl[1])
                spr = []
                valn = ""
                splr = csl[4].split("-")
                for pr in splr:
                    spr.append(self.nodos[pr][2])
                try:
                    valn = self.nodoTransportista[splr[-1]]
                except:
                    pass
                ws.write(lcnt, 3, valn, sts)
                uaLinea.append(valn)
                ws.write(lcnt, 4, csl[2], sts)
                uaLinea.append(csl[2])
                vr = csl[3]
                ws.write(lcnt, 5, self.nodos[vr][0], sts)
                uaLinea.append(self.nodos[vr][0])
                ws.write(lcnt, 6, self.varNodo[vr], sts)
                uaLinea.append(vr)
                ws.write(lcnt, 7, self.nodos[vr][2], sts)
                uaLinea.append(self.nodos[vr][2])

                ws.write(lcnt, 8, " - ".join(spr), sts)
                uaLinea.append(" - ".join(spr))
                dtFecha = datetime.datetime.strptime(self.fecha.strftime('%Y%m%d'), '%Y%m%d')
                nFecha = dtFecha + datetime.timedelta(seconds=int(csl[5]))
                ws.write(lcnt, 9, nFecha.date(), stsDate)
                ws.write(lcnt, 10, nFecha.time(), stsTime)
                uaLinea.append(nFecha)
                uaLinea.append(int(csl[6])-int(csl[5]))
                nFecha = dtFecha + datetime.timedelta(seconds=int(csl[6]))
                ws.write(lcnt, 11, nFecha.date(), stsDate)
                ws.write(lcnt, 12, nFecha.time(), stsTime)
                separador = ""
                if csl[7] == "S":
                    separador = "si"
                ws.write(lcnt, 13, separador, sts)
                uaLinea.append(separador)
                compartido = ""
                if csl[8] == "C":
                    compartido = "compartido"
                ws.write(lcnt, 14, compartido, sts)
                uaLinea.append(compartido)
                ws.write(lcnt, 15, csl[9].capitalize(), sts)
                ws.write(lcnt, 16, csl[10].upper(), sts)
                ws.write(lcnt, 17, float(csl[11]), stsNum)
                ws.write(lcnt, 18, float(csl[12]), stsNum)
                x= "X"
                if csl[0]+csl[3]+csl[4] in llaves:
                    x=""
                else:
                    uaLinea.append(csl[13].capitalize())
                    uAndenes.append(uaLinea)

                ws.write(lcnt, 19, x, stsNum)
                llaves.add(csl[0]+csl[3]+csl[4])
                ws.write(lcnt, 20, csl[13].capitalize(), sts)
            lcnt += 1
        
        ws = wb.add_sheet("Utilización Andenes")
        ws.write(0, 0, "Fecha de Programa", stsb)
        ws.write(0, 1, "Número Camión", stsb)
        ws.write(0, 2, "tipo Camión", stsb)
        ws.write(0, 3, "Agente Servicios", stsb)
        ws.write(0, 4, "Tipo Viaje", stsb)
        ws.write(0, 5, "Tipo Nodo", stsb)
        ws.write(0, 6, "Nodo", stsb)
        ws.write(0, 7, "Nombre Nodo", stsb)
        ws.write(0, 8, "Ruta", stsb)
        ws.write(0, 9, "Fecha", stsb)        
        ws.write(0, 10, "Hora", stsb)
        ws.write(0, 11, "Separador", stsb)
        ws.write(0, 12, "Compartido", stsb)
        ws.write(0, 13, "Inicio", stsb)        
        ws.write(0, 14, "Operación", stsb) 
        lcnt = 1
        for lin in uAndenes: 
            for e in range(0, int(int(lin[10])/1800)+1):
                ws.write(lcnt, 0, lin[0], stsDate)
                ws.write(lcnt, 1, lin[1], sts)                
                ws.write(lcnt, 2, lin[2].replace("P", " P"), sts)
                ws.write(lcnt, 3, lin[3], sts)
                ws.write(lcnt, 4, lin[4], sts)
                ws.write(lcnt, 5, lin[5], sts)
                ws.write(lcnt, 6, self.varNodo[lin[6]], sts)
                ws.write(lcnt, 7, lin[7], sts)
                ws.write(lcnt, 8, lin[8], sts)
                nFecha = lin[9] + datetime.timedelta(seconds=1800*e)
                ws.write(lcnt, 9, nFecha.date(), stsDate)
                ws.write(lcnt, 10, nFecha.time(), stsTime)
                ws.write(lcnt, 11, lin[11], sts)
                ws.write(lcnt, 12, lin[12], sts)
                inicio = ""
                if e == 0:
                    inicio = "X"
                ws.write(lcnt, 13, inicio, sts)
                ws.write(lcnt, 14, lin[13], sts)
                lcnt += 1
        wb.save(self.archivoIndicadores)

    def grabarDetalle(self):
        print("-Grabando Detalle Carga-")
        sts = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;')
        stsb = xlwt.easyxf('font: bold true, colour black; borders: left thin, right thin, top thin, bottom thin;')
        wb = xlwt.Workbook()
        #arch = open(self.pathTmpLogistica + "/SalidaDetalle.txt","r") 
        #ws = wb.add_sheet("Detalle")
        #lcnt = 0
        #cabecera = ["Fecha", "Nro. Camión", "Tipo Viaje", "Id Pallet", "Sector", "Estado", "Material", "Descripción", "Nodo Origen", "Nombre Origen", "Nodo Destino", "Nombre Destino", "Cajas"]
        #for lidx in arch.readlines():
        #    ccnt = 0
        #    if lcnt == 0:
        #        for e in cabecera:
        #            ws.write(lcnt, ccnt, e, stsb)
        #            ccnt += 1
        #    else:
        #        csl = lidx.split(";")[0:-1]
        #        ws.write(lcnt, ccnt, self.fecha.strftime('%d/%m/%Y'), sts)
        #        ccnt += 1
        #        for pcel in range(0, len(csl)-1):
        #            cel = csl[pcel]
        #            if pcel == 6:
        #                continue
        #            if ccnt == 4:
        #                ws.write(lcnt, ccnt, self.materiales[int(cel)].sector, sts)
        #                ws.write(lcnt, ccnt + 1, self.materiales[int(cel)].estado, sts)
        #                ccnt += 2
        #            celf = cel
        #            if cel in self.varNodo.keys():
        #                celf = self.varNodo[cel]
        #            try:
        #                celf = float(cel)
        #            except:
        #                pass                        
        #            ws.write(lcnt, ccnt, celf, sts)                    
        #            if ccnt == 8 or ccnt == 10:
        #                vr = cel
        #                if not cel in self.nodos.keys():
        #                    vr = int(cel)
        #                if vr == 60472:
        #                    vr = "P" + str(vr)                 
        #                ccnt += 1
        #                ccs = ""
        #                if ccnt == 11:
        #                    try:
        #                        if vr == "P60472":
        #                            vr = 60472
        #                        if csl[6] == "S":
        #                            ws.write(lcnt, ccnt, self.destinatariosMercOperZDM[vr][-1], sts)
        #                        else:
        #                            ws.write(lcnt, ccnt, self.destinatariosMercOperZDM[vr][0], sts)
        #                    except:
        #                        ws.write(lcnt, ccnt, self.nodos[vr][2], sts)
        #                if ccnt == 9:
        #                    ws.write(lcnt, ccnt, self.nodos[vr][2], sts)
        #            if ccnt == 6:
        #                ccnt += 1
        #                ws.write(lcnt, ccnt, self.materiales[int(cel)].descripcion, sts)
        #            ccnt += 1
        #    lcnt += 1

            
        arch = open(self.pathTmpLogistica + "/SalidaDetalle.txt","r") 
        ws = wb.add_sheet("Detalle")
        lcnt = 0
        cabecera = ["Fecha", "Nro. Camión", "Tipo Viaje", "Id Pallet", "Centro Descarga", "Sector", "Estado", "Material", "Descripción", "Nodo Origen", "Nombre Origen", "Nodo Destino", "Nombre Destino", "Cajas"]
        for lidx in arch.readlines():
            ccnt = 0
            if lcnt == 0:
                for e in cabecera:
                    ws.write(lcnt, ccnt, e, stsb)
                    ccnt += 1
            else:
                csl = lidx.split(";")[0:-1]
                ws.write(lcnt, ccnt, self.fecha.strftime('%d/%m/%Y'), sts)
                ccnt += 1
                for pcel in range(0, len(csl)-1):
                    cel = csl[pcel]
                    if pcel == 6:
                        continue
                    if ccnt == 4:
                        ws.write(lcnt, ccnt, self.nodos[csl[-1]][2], sts)
                        ccnt += 1
                        ws.write(lcnt, ccnt, self.materiales[int(cel)].sector, sts)
                        ws.write(lcnt, ccnt + 1, self.materiales[int(cel)].estado, sts)
                        ccnt += 2
                    celf = cel
                    if cel in self.varNodo.keys():
                        celf = self.varNodo[cel]
                    try:
                        celf = float(cel)
                    except:
                        pass                        
                    ws.write(lcnt, ccnt, celf, sts)                    
                    if ccnt == 9 or ccnt == 11:
                        vr = cel
                        if not cel in self.nodos.keys():
                            vr = int(cel)
                        if vr == 60472:
                            vr = "P" + str(vr)                 
                        ccnt += 1
                        ccs = ""
                        if ccnt == 12:
                            try:
                                if vr == "P60472":
                                    vr = 60472
                                if csl[6] == "S":
                                    ws.write(lcnt, ccnt, self.destinatariosMercOperZDM[vr][-1], sts)
                                else:
                                    ws.write(lcnt, ccnt, self.destinatariosMercOperZDM[vr][0], sts)
                            except:
                                ws.write(lcnt, ccnt, self.nodos[vr][2], sts)
                        if ccnt == 10:
                            ws.write(lcnt, ccnt, self.nodos[vr][2], sts)
                    if ccnt == 7:
                        ccnt += 1
                        ws.write(lcnt, ccnt, self.materiales[int(cel)].descripcion, sts)
                    ccnt += 1
            lcnt += 1            


        arch = open(self.pathTmpLogistica + "/SalidaDetalle2.txt","r") 
        ws = wb.add_sheet("Detalle2")
        lcnt = 0
        jhsetKeys = self.JHSET.keys()
        ddCajasKeys = self.ddCajas.keys() 
        ddaModelo = set()
        cabecera = ["Fecha", "Tipo Viaje", "Sector", "Estado", "Env-Granel", "Set", "Material", "Descripción", "Nodo Destino", "Nombre Destino", "Input"]
        for lidx in arch.readlines():
            ccnt = 0
            nPlantas = 0
            csl = lidx.split(";")[0:-1]
            if lcnt == 0:
                for e in cabecera:
                    ws.write(lcnt, ccnt, e, stsb)
                    ccnt += 1
                
                for e in csl[4:]:
                    ws.write(lcnt, ccnt, self.varNodo[e], stsb)
                    ccnt += 1
            else:
                ws.write(lcnt, ccnt, self.fecha.strftime('%d/%m/%Y'), sts)
                ws.write(lcnt, ccnt + 1, csl[0], sts)
                ws.write(lcnt, ccnt + 2, self.materiales[int(csl[1])].sector, sts)
                ws.write(lcnt, ccnt + 3, self.materiales[int(csl[1])].estado, sts)
                ws.write(lcnt, ccnt + 4, self.materiales[int(csl[1])].envgranel, sts)
                if (csl[2]+csl[3], int(csl[1])) in jhsetKeys:
                    ws.write(lcnt, ccnt + 5, "TRUE", sts)
                else:
                    ws.write(lcnt, ccnt + 5, "FALSE", sts)
                ws.write(lcnt, ccnt + 6, int(csl[1]), sts)
                ws.write(lcnt, ccnt + 7, self.materiales[int(csl[1])].descripcion, sts)
                cel = csl[2]
                celf = cel
                if cel in self.varNodo.keys():
                    celf = self.varNodo[cel]                
                ws.write(lcnt, ccnt + 8, celf, sts)                    
                vr = cel
                if not cel in self.nodos.keys():
                    try:
                        vr = int(cel)
                    except:
                        pass                        
                try:
                    if csl[3] == "S":
                        ws.write(lcnt, ccnt + 9, self.destinatariosMercOperZDM[vr][-1], sts)
                    else:
                        ws.write(lcnt, ccnt + 9, self.destinatariosMercOperZDM[vr][0], sts)
                except:
                    ws.write(lcnt, ccnt + 9, self.nodos[vr][2], sts)
                ddaModelo.add((csl[2]+csl[3], int(csl[1])))
                if (csl[2]+csl[3], int(csl[1])) in ddCajasKeys:
                    ws.write(lcnt, ccnt + 10, self.ddCajas[csl[2]+csl[3], int(csl[1])], sts)
                for e in csl[4:]:
                    if e != "":
                        ws.write(lcnt, ccnt + 11, int(e), sts)
                    else:
                        ws.write(lcnt, ccnt + 11, 0, sts)
                    ccnt += 1
                    nPlantas += 1
            lcnt += 1
        difDda = ddCajasKeys - ddaModelo
        for e in difDda:
            ccnt = 0
            ws.write(lcnt, ccnt, self.fecha.strftime('%d/%m/%Y'), sts)
            ws.write(lcnt, ccnt + 1, "primario", sts) #"insatisfecho", sts)
            ws.write(lcnt, ccnt + 2, self.materiales[e[1]].sector, sts)
            ws.write(lcnt, ccnt + 3, self.materiales[e[1]].estado, sts)
            ws.write(lcnt, ccnt + 4, self.materiales[e[1]].envgranel, sts)
            if e in jhsetKeys:
                ws.write(lcnt, ccnt + 5, "TRUE", sts)
            else:
                ws.write(lcnt, ccnt + 5, "FALSE", sts)
            ws.write(lcnt, ccnt + 6, e[1], sts)
            ws.write(lcnt, ccnt + 7, self.materiales[e[1]].descripcion, sts)
            if e[0][-1] == "S":
                cel = e[0][:-1]
            else:
                cel = e[0]
            if cel in self.varNodo.keys():
                celf = self.varNodo[cel]            
            ws.write(lcnt, ccnt + 8, celf, sts)                    
            vr = cel
            if not cel in self.nodos.keys():
                try:
                    vr = int(cel)
                except:
                    pass                        
            try:
                if e[0][-1] == "S":
                    ws.write(lcnt, ccnt + 9, self.destinatariosMercOperZDM[vr][-1], sts)
                else:
                    ws.write(lcnt, ccnt + 9, self.destinatariosMercOperZDM[vr][0], sts)
            except:
                ws.write(lcnt, ccnt + 9, self.nodos[vr][2], sts)
            
            ws.write(lcnt, ccnt + 10, self.ddCajas[e[0], e[1]], sts)
            for i in range(0, nPlantas):
                ws.write(lcnt, ccnt + 11, 0, sts)
                ccnt += 1
            lcnt += 1
        wb.save(self.archivoDetalle)

    def imprimirLineaTraslado(self, lcnt, sap, lin, sts, ws, stsDate):
        ws.write(lcnt, 0, sap[lin[0]]["Clase Pedido"], sts)
        ws.write(lcnt, 2, sap[lin[0]]["Org Compra"], sts)
        ws.write(lcnt, 3, sap[lin[0]]["Grupo Compra"], sts)
        ws.write(lcnt, 4, self.varNodo[lin[3]], sts)
        if lin[4] == "S":
            ws.write(lcnt, 6, "CCS_" + self.varNodo[lin[3]], sts)
        else:
            ws.write(lcnt, 6, self.varNodo[lin[3]], sts)
        ws.write(lcnt, 8, int(lin[1]), sts)
        ws.write(lcnt, 9, int(lin[5]), sts)
        ws.write(lcnt, 10, "Cj", sts)
        ws.write(lcnt, 11, self.varNodo[lin[2]], sts)
        ws.write(lcnt, 12, self.fecha, stsDate) 
        try:
            dtFecha = datetime.datetime.strptime(self.fecha.strftime('%Y%m%d'), '%Y%m%d')
            nFecha = dtFecha + datetime.timedelta(seconds=int(lin[6]))        
            ws.write(lcnt, 14, nFecha, stsDate) 
        except:
            pass                        

    def imprimirLineaVenta(self, lcnt, sap, lin, sts, ws, stsDate):
        ws.write(lcnt, 0, sap[lin[0]]["Clase Pedido"], sts)
        ws.write(lcnt, 2, sap[lin[0]]["Z1"], sts)
        ws.write(lcnt, 3, self.varNodo[lin[3]], sts)
        ws.write(lcnt, 4, sap[lin[0]]["Z2"], sts)
        ws.write(lcnt, 7, int(lin[1]), sts)
        ws.write(lcnt, 8, int(lin[5]), sts)
        ws.write(lcnt, 9, "Cj", sts)
        ws.write(lcnt, 10, self.varNodo[lin[2]], sts)
        ws.write(lcnt, 11, self.fecha, stsDate)

    def imprimirLineaCabecera(self, lcnt, sap, lin, sts, ws, stsDate):
        ws.write(lcnt, 0, int(lin[0]), sts)
        ws.write(lcnt, 1, sap[lin[3]]["Clase Transp."], sts)
        ws.write(lcnt, 2, sap[lin[3]]["PPTransp."], sts)
        ws.write(lcnt, 4, self.fecha, stsDate) 
        ws.write(lcnt, 5, lin[1].replace("P", " P"), sts)
        ws.write(lcnt, 6, lin[2].replace("S", "Si"), sts)
        spr = []
        descargas = ""
        stl = lin[4].split("-")
        for pr in stl:
            vr = pr 
            if not pr in self.nodos.keys():
                vr = int(pr)
            if vr == 60472:
                vr = "P" + str(int(vr))
            spr.append(self.nodos[vr][2])
        pv = 1
        for e in range(1, len(spr)):
            if stl[e][0:2] == "Su":
                descargas += (str(pv) + "º Descarga: " + spr[e] + " - ")
                pv += 1
        if pv <= 2:
            descargas = ""
        try:
            ws.write(lcnt, 3, self.nodoTransportista[stl[-1]], sts)
        except:
            pass
        ws.write(lcnt, 7, spr[-1], sts)
        ws.write(lcnt, 8, descargas[:-3], sts)
        ws.write(lcnt, 11, sap[lin[3]]["Ruta"], sts)

    def imprimirLineaDetalle(self, lcnt, sap, lin, sts, ws, stsDate, stsTime):
        ws.write(lcnt, 0, int(lin[0]), sts)
        vr = lin[1]
        if not vr in self.nodos.keys():
            try:
                vr = int(vr)
            except:
                pass                        
        ws.write(lcnt, 1, self.nodos[vr][5], sts)
        ws.write(lcnt, 2, lin[4], sts)
        tSup = datetime.time(0, 0, 59)
        tInf = datetime.time(0, 0, 0)
        dtFecha = datetime.datetime.strptime(self.fecha.strftime('%Y%m%d'), '%Y%m%d')
        nFecha = dtFecha + datetime.timedelta(seconds=int(lin[2]))
        ws.write(lcnt, 3, nFecha.date(), stsDate)
        nFT = nFecha.time()
        if nFT >= tInf and nFT <= tSup:
            nFT = (nFecha + datetime.timedelta(seconds=60)).time()
        ws.write(lcnt, 4, timeRound(nFT), stsTime)
        nFecha = dtFecha + datetime.timedelta(seconds=int(lin[3]))
        ws.write(lcnt, 5, nFecha.date(), stsDate)
        nFT = nFecha.time()
        if nFT >= tInf and nFT <= tSup:
            nFT = (nFecha + datetime.timedelta(seconds=60)).time()
        ws.write(lcnt, 6, timeRound(nFT), stsTime)
        for e in range(5, len(lin)):
            try:
                ws.write(lcnt, e+2, float(lin[e]), sts)
            except:
                pass
                
    def grabarExportSAP(self):
        print("-Grabando Export SAP-")
        cabeceras = {
        "PedTraCg": {"Hoja1":["Clase Pedido", "", "Org Compra", "Grupo Compra", "Centro Destino", "", "Destino", "", "Material", "Cajas", "Unidad", "Centro Origen", "Fecha", "", "Suministro"]},
        "PedTraFr": {"Hoja1":["Clase Pedido", "", "Org Compra", "Grupo Compra", "Centro Destino", "", "Destino", "", "Material", "Cajas", "Unidad", "Centro Origen", "Fecha", "", "Suministro"]},
        "PedVtaCg": {"Hoja1":["Clase Pedido", "Oferta", "Z1", "Centro Destino", "Z2", "", "", "Material", "Cajas", "Unidad", "Centro Origen", "Fecha"]}, 
        "PedVtaFr": {"Hoja1":["Clase Pedido", "Oferta", "Z1", "Centro Destino", "Z2", "", "", "Material", "Cajas", "Unidad", "Centro Origen", "Fecha"]}, 
        "transportes": {"Cabecera":["Corr. Camión", "Clase Transp.", "PPTransp.", "Emp. Transp.", "Fecha", "Tipo Camión", "Separador", "ZDMO", "Observaciones", "Otro 1", "Otro 2", "Ruta"],
                        "Detalle":["Corr. Camión", "Nodo", "Camion-Entrega", "Fec.Lleg.", "Hr.Lleg.", "Fec.Sal.", "Hr.Sal.", "Po C", "Cer C", "El C", "Sa C", "Pa C", "Ho C", "Po F", "Ce F", "Pa F", "Ce", "Sa F", "Vin", "Vacun", "Otro 4", "Otro 5", "Otro 6"]
                        },
        } 

        lineas = []
        arch = open(self.pathTmpLogistica + "/venta-traslado.txt","r") 
        cnt = 0
        for lidx in arch.readlines():
            if cnt > 0:
                lineas.append(lidx.split(";"))
            cnt += 1
  
        sts = xlwt.easyxf('font: colour black;')
        #stsDate = xlwt.easyxf('font: colour black;', num_format_str='DD-MM-YYYY')
        stsDate = xlwt.easyxf('font: colour black;', num_format_str='DD.MM.YYYY')
        stsTime = xlwt.easyxf('font: colour black;', num_format_str='HH:mm')
        stsb = xlwt.easyxf('font: bold true, colour black;')
        wss = []
        for cab in cabeceras:
            wb = xlwt.Workbook()
            for hoj in cabeceras[cab].keys():
                ws = wb.add_sheet(hoj)
                wss.append(ws)
                ccnt = 0
                for col in cabeceras[cab][hoj]:
                    ws.write(0, ccnt, col, stsb)
                    ccnt += 1
                lcnt = 1
                sap = None
                for lin in lineas:
                    vr = lin[3]
                    if not vr in self.nodos.keys():
                        try:
                            vr = int(vr)
                        except:
                            pass                        
                    #lin[3] = lin[3].replace("T1632", "T163_2").replace("P60472", "60472")
                    if cab[3:6] == "Tra" and self.nodos[vr][0] != "Venta Directa":
                        sap = self.sap["SAP2"]
                        if cab[6:] == "Cg" and self.materiales[int(lin[1])].estado == "Congelado":
                            self.imprimirLineaTraslado(lcnt, sap, lin, sts, ws, stsDate)
                            lcnt += 1
                        if cab[6:] == "Fr" and self.materiales[int(lin[1])].estado == "Refrigerado":
                            self.imprimirLineaTraslado(lcnt, sap, lin, sts, ws, stsDate)
                            lcnt += 1
                    elif cab[3:6] == "Vta" and self.nodos[vr][0] == "Venta Directa":
                        sap = self.sap["SAP3"]                    
                        if cab[6:] == "Cg" and self.materiales[int(lin[1])].estado == "Congelado":
                            self.imprimirLineaVenta(lcnt, sap, lin, sts, ws, stsDate)
                            lcnt += 1
                        if cab[6:] == "Fr" and self.materiales[int(lin[1])].estado == "Refrigerado":
                            self.imprimirLineaVenta(lcnt, sap, lin, sts, ws, stsDate)
                            lcnt += 1
                if cab == "transportes":
                    sap = self.sap["SAP1"]
                    lineasT = []
                    lcnt = 1
                    if hoj == "Cabecera":
                        arch = open(self.pathTmpLogistica + "/transportes-cabecera.txt","r") 
                        cnt = 0
                        for lidx in arch.readlines():
                            if cnt > 0:
                                lineasT.append(lidx.split(";"))
                            cnt += 1
                        for lin in lineasT:
                            self.imprimirLineaCabecera(lcnt, sap, lin, sts, ws, stsDate)
                            lcnt += 1
                    else:
                        arch = open(self.pathTmpLogistica + "/transportes-detalle.txt","r") 
                        cnt = 0
                        for lidx in arch.readlines():
                            if cnt > 0:
                                lineasT.append(lidx.split(";"))
                            cnt += 1
                        for lin in lineasT:
                            self.imprimirLineaDetalle(lcnt, sap, lin, sts, ws, stsDate, stsTime)
                            lcnt += 1
                    
            wb.save(self.pathSapLogistica + "/" + cab + "_" + self.fecha.strftime('%d.%m.%Y') + ".xls")
            
    def convertirXLSaCSV(self, xls, csv):
        
        pythoncom.CoInitialize()
        excel = win32.gencache.EnsureDispatch('Excel.Application') # Calls Excel
        excel.DisplayAlerts = False # Disables prompts, such as asking to overwrite files
        wb = excel.Workbooks.Open(xls.replace('/', '\\')) # Input File
        wb.SaveAs(csv.replace('/', '\\'), win32.constants.xlCSV) # Output File
        wb.Close()
        excel.Application.Quit() # Close Excel
        excel.DisplayAlerts = True # Turn alerts back on  
        os.remove(xls)      
        '''
        workbook = xlrd.open_workbook(xls)
        all_worksheets = workbook.sheet_names()
        for worksheet_name in all_worksheets:
            worksheet = workbook.sheet_by_name(worksheet_name)
            with open(u'{}.csv'.format(worksheet_name), 'w', encoding="utf-8") as csvf:
                wr = csv.writer(csvf, quoting=csv.QUOTE_ALL)
                for rownum in range(worksheet.nrows):
                    wr.writerow(worksheet.row_values(rownum))        
        #os.remove(xls)
        '''

    def convertirXLSaXLSX(self, xls, xlsx):
        
        pythoncom.CoInitialize()
        excel =  win32.Dispatch('Excel.Application') #win32.gencache.EnsureDispatch('Excel.Application') # Calls Excel
        excel.DisplayAlerts = False # Disables prompts, such as asking to overwrite files
        wb = excel.Workbooks.Open(xls.replace('/', '\\')) # Input File
        wb.SaveAs(xlsx.replace('/', '\\'), FileFormat = 51) # Output File
        wb.Close()
        excel.Application.Quit() # Close Excel
        excel.DisplayAlerts = True # Turn alerts back on  
        os.remove(xls) 
        '''
        sheet0 = pyexcel.get_sheet(file_name=xls, name_columns_by_row=0)
        xlsarray = sheet0.to_array() 
        sheet1 = pyexcel.Sheet(xlsarray)
        sheet1.save_as(xlsx)
        #os.remove(xls) 
        '''

    def imprimirLineaZlogTransportes(self, ws, sts, stsDate, stsTime, stsNum, llegadasCamiones, csl, splr, valn, vr, lcnt, remplazoPlanta=None, esPSS=False, llegadasCamionesPL=None):
        ws.write(lcnt, 0, self.fecha, stsDate)
        if remplazoPlanta is None:
            ws.write(lcnt, 1, self.nodos[splr[-1]][2] , sts)
        else:
            ws.write(lcnt, 1, self.nodos[vr][2] , sts)
        if remplazoPlanta is None:
            dtFecha = datetime.datetime.strptime(self.fecha.strftime('%Y%m%d'), '%Y%m%d')
            nFecha = dtFecha + datetime.timedelta(seconds=int(llegadasCamiones[csl[0]]))
        else:            
            dtFecha = datetime.datetime.strptime(self.fecha.strftime('%Y%m%d'), '%Y%m%d')
            nFecha = dtFecha + datetime.timedelta(seconds=int(csl[5]))
        ws.write(lcnt, 2, nFecha.date(), stsDate)
        ws.write(lcnt, 3, nFecha.time(), stsTime)
        ws.write(lcnt, 4, valn, sts)
        ws.write(lcnt, 5, float(csl[0]), sts)
        if remplazoPlanta is None:
            ws.write(lcnt, 6, self.varNodo[vr], sts)
        else:
            ws.write(lcnt, 6, self.varNodo[remplazoPlanta], sts)
        ws.write(lcnt, 7, self.sectores[csl[9]], sts)

        if not esPSS:
            dtFecha = datetime.datetime.strptime(self.fecha.strftime('%Y%m%d'), '%Y%m%d')
            nFecha = dtFecha + datetime.timedelta(seconds=int(csl[5]))
        else:
            dtFecha = datetime.datetime.strptime(self.fecha.strftime('%Y%m%d'), '%Y%m%d')
            nFecha = dtFecha + datetime.timedelta(seconds=int(llegadasCamionesPL[csl[0]][0]))
        ws.write(lcnt, 8, nFecha.date(), stsDate)
        ws.write(lcnt, 9, nFecha.time(), stsTime)
        if not esPSS:
            nFecha = dtFecha + datetime.timedelta(seconds=int(csl[6]))
        else:
            nFecha = dtFecha + datetime.timedelta(seconds=int(llegadasCamionesPL[csl[0]][1]))

        ws.write(lcnt, 10, nFecha.date(), stsDate)
        ws.write(lcnt, 11, nFecha.time(), stsTime)
        ws.write(lcnt, 12, float(csl[11]), stsNum)
        ws.write(lcnt, 13, str(int(self.estadosMaterial[csl[10].replace("Fresco", "refrigerado").lower()])).zfill(5) , sts)
        ws.write(lcnt, 14, csl[10].replace("Fresco", "refrigerado").upper(), sts)
        separador = ""
        if csl[7] == "S":
            separador = "mixto"
        else:
            separador = csl[10].lower()
        ws.write(lcnt, 15, separador, sts)
        ws.write(lcnt, 16, float(csl[12]), stsNum)
        compartido = ""
        if csl[8] == "C":
            compartido = "compartido"
        ws.write(lcnt, 17, compartido, sts)
        ws.write(lcnt, 18, csl[1].replace("P", " P"), sts)
        ws.write(lcnt, 19, "", sts)
        ws.write(lcnt, 20, "", sts)
        spr = []
        descargas = ""
        for pr in splr:
            vr = pr 
            if not pr in self.nodos.keys():
                vr = int(pr)
            if vr == 60472:
                vr = "P" + str(int(vr))
            spr.append(self.nodos[vr][2])
        pv = 1
        for e in range(1, len(spr)):
            if splr[e][0:2] == "Su":
                descargas += (str(pv) + "º Descarga: " + spr[e] + " - ")
                pv += 1
        if pv <= 2:
            descargas = ""                
        ws.write(lcnt, 21, descargas, sts)
        interp = ""
        if csl[2] == "interplanta":
            interp = "X"
        ws.write(lcnt, 22, interp, sts)

    def grabarZlogTransportes(self):
        print("-Grabando Zlog-Transportes-")
        sts = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;')
        stsNum = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;', num_format_str = '#,##0.000')
        stsb = xlwt.easyxf('font: bold true, colour black; borders: left thin, right thin, top thin, bottom thin;')
        wb = xlwt.Workbook()    
        stsDate = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;', num_format_str='DD/MM/YYYY')
        stsTime = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;', num_format_str='HH:mm:ss')
        ws = wb.add_sheet("Transportes")
        ws.write(0, 0, "Fecha de Despacho", stsb)
        ws.write(0, 1, "Destino", stsb)
        ws.write(0, 2, "Fecha Destino", stsb)        
        ws.write(0, 3, "Hora Destino", stsb)
        ws.write(0, 4, "Transportista", stsb)
        ws.write(0, 5, "N° Camión SAP", stsb)
        ws.write(0, 6, "Planta", stsb)
        ws.write(0, 7, "Tipo Material", stsb)
        ws.write(0, 8, "Fecha Citación", stsb)
        ws.write(0, 9, "Hora Citación", stsb)
        ws.write(0, 10, "Fecha Salida", stsb)
        ws.write(0, 11, "Hora Salida", stsb)
        ws.write(0, 12, "Cantidad Pallet", stsb)
        ws.write(0, 13, "Estado", stsb)        
        ws.write(0, 14, "Descripción Estado", stsb)
        ws.write(0, 15, "Separador", stsb)
        ws.write(0, 16, "Cantidad de Kilos", stsb)
        ws.write(0, 17, "Compartido", stsb)
        ws.write(0, 18, "Tipo de Camión", stsb)
        ws.write(0, 19, "Ruta", stsb)
        ws.write(0, 20, "Estado Carga", stsb)
        ws.write(0, 21, "Observaciones", stsb)
        ws.write(0, 22, "Interplanta", stsb)
        llegadasCamiones = {}
        llegadasCamionesPL = {}
        camionRuta = {}
        idxDest = open(self.pathTmpLogistica + "/transportes-detalle.txt", "r") 
        for lidx in idxDest.readlines():
            csl = lidx.split(";")
            llegadasCamiones[csl[0]] = csl[2]
            if csl[1].startswith("Pl"):
                llegadasCamionesPL[csl[0]] = [csl[2], csl[3]]
        idx = open(self.pathTmpLogistica + "/capacidades.txt", mode="r", encoding="utf-8") 
        lcnt = 0
        llaves = set()
        uAndenes = []
        for lidx in idx.readlines():
            csl = lidx.split(";")[0:-1]
            if lcnt > 0:
                vr = csl[3]
                spr = []
                valn = ""
                splr = csl[4].split("-")
                if not csl[0] in camionRuta.keys() and len(splr) == 3 and vr==splr[0] and not splr[1].startswith("Pl") and not splr[2].startswith("Pl"):
                    camionRuta[csl[0]] = {}
                    camionRuta[csl[0]][splr[1]] = []
                    camionRuta[csl[0]][splr[2]] = []
                    continue

                if csl[0] in camionRuta.keys() and len(splr) == 3 and vr==splr[0] and not splr[1].startswith("Pl") and not splr[2].startswith("Pl"):
                    continue

                if csl[0] in camionRuta.keys() and csl[3] in camionRuta[csl[0]].keys():
                    camionRuta[csl[0]][csl[3]].append(csl)
                    continue
                else:
                    for pr in splr:
                        spr.append(self.nodos[pr][2])
                    try:
                        valn = self.nodoTransportista[splr[-1]]
                    except:
                        pass
                    if (csl[3].startswith("Pl") and csl[3] != splr[1]) or (csl[3].startswith("Pl") and csl[3] == splr[1] and len(splr) != 2):
                        self.imprimirLineaZlogTransportes(ws, sts, stsDate, stsTime, stsNum, llegadasCamiones, csl, splr, valn, vr, lcnt)
                        lcnt += 1
            else:
                 lcnt += 1
                 
        for i in camionRuta.keys():
            for j in camionRuta[i].keys():
                for csl in camionRuta[i][j]:
                    vr = csl[3]
                    spr = []
                    valn = ""
                    splr = csl[4].split("-")
                    for pr in splr:
                        spr.append(self.nodos[pr][2])
                    try:
                        valn = self.nodoTransportista[splr[-1]]
                    except:
                        pass
                    self.imprimirLineaZlogTransportes(ws, sts, stsDate, stsTime, stsNum, llegadasCamiones, csl, splr, valn, vr, lcnt, splr[0])
                    lcnt += 1

        wb.save(self.pathSapLogistica + "/ZLOG_Transportes.xls")
        self.convertirXLSaXLSX(self.pathSapLogistica + "/ZLOG_Transportes.xls", self.pathSapLogistica + "/ZLOG_Transportes.xlsx")


    def grabarZlogSuministros(self):
        print("-Grabando Zlog-Suministros-")
        sts = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;')
        stsb = xlwt.easyxf('font: bold true, colour black; borders: left thin, right thin, top thin, bottom thin;')
        wb = xlwt.Workbook()    
        ws = wb.add_sheet("Suministros")
        ws.write(0, 0, "Fecha", stsb)
        ws.write(0, 1, "Tipo Viaje", stsb)
        ws.write(0, 2, "Sector", stsb)        
        ws.write(0, 3, "Estado", stsb)
        ws.write(0, 4, "Material", stsb)
        ws.write(0, 5, "Descripción", stsb)
        ws.write(0, 6, "Nodo Origen", stsb)
        ws.write(0, 7, "Nombre Origen", stsb)
        ws.write(0, 8, "Nodo Destino", stsb)
        ws.write(0, 9, "Nombre Destino", stsb)
        ws.write(0, 10, "Total", stsb)
        pPlantas = []
        lcnt = 0
        arch = open(self.pathTmpLogistica + "/SalidaDetalle2.txt","r")         
        for lidx in arch.readlines():
            csl = lidx.split(";")[0:-1]
            if lcnt == 0:
                pPlantas = csl
                lcnt = 1
            else:
                total = 0
                pos = 0
                for e in reversed(csl):
                    pos += 1
                    if pos > 7:
                        break
                    if len(e) > 0:
                        total = e
                        ws.write(lcnt, 0, self.fecha.strftime('%d/%m/%Y'), sts)
                        ws.write(lcnt, 1, csl[0], sts)
                        ws.write(lcnt, 2, self.materiales[int(csl[1])].sector, sts)
                        ws.write(lcnt, 3, self.materiales[int(csl[1])].estado, sts)
                        ws.write(lcnt, 4, int(csl[1]), sts)
                        ws.write(lcnt, 5, self.materiales[int(csl[1])].descripcion, sts)
                            
                        ws.write(lcnt, 6, self.varNodo[pPlantas[-pos]], sts)
                        ws.write(lcnt, 7, self.nodos[pPlantas[-pos]][2], sts)
                        ws.write(lcnt, 8, self.varNodo[csl[2]], sts)
                        try:
                            if csl[3] == "S":
                                ws.write(lcnt, 9, self.destinatariosMercOperZDM[csl[2]][-1], sts)
                            else:
                                ws.write(lcnt, 9, self.destinatariosMercOperZDM[csl[2]][0], sts)
                        except:
                            ws.write(lcnt, 9, self.nodos[csl[2]][2], sts)
                        ws.write(lcnt, 10, int(total), sts)
                        lcnt += 1

        wb.save(self.pathSapLogistica + "/ZLOG_Suministros.xls")
        self.convertirXLSaCSV(self.pathSapLogistica + "/ZLOG_Suministros.xls", self.pathSapLogistica + "/ZLOG_Suministros.csv")
