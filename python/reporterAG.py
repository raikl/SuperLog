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
    def __init__(self, srcRep, nodos, sap, sectores, materiales, estadosMaterial, nodoTransportista, fecha, carpetaSalida):
        self.srcRep = srcRep
        self.nodos = nodos
        self.sap = sap
        self.sectores = sectores
        self.materiales = materiales
        self.estadosMaterial = estadosMaterial
        self.nodoTransportista = nodoTransportista
        self.fecha = fecha
        self.pathSap = carpetaSalida + "/SAP"

    def imprimirLineaTraslado(self, lcnt, sap, lin, sts, ws, stsDate):
        ws.write(lcnt, 0, sap[lin[0]]["Clase Pedido"], sts)
        ws.write(lcnt, 2, sap[lin[0]]["Org Compra"], sts)
        ws.write(lcnt, 3, sap[lin[0]]["Grupo Compra"], sts)
        ws.write(lcnt, 4, lin[3][2:], sts)
        if lin[4] == "S":
            ws.write(lcnt, 6, "CCS_" + lin[3][2:], sts)
        else:
            ws.write(lcnt, 6, lin[3][2:], sts)
        ws.write(lcnt, 8, int(lin[1]), sts)
        ws.write(lcnt, 9, int(lin[5]), sts)
        ws.write(lcnt, 10, "Cj", sts)
        ws.write(lcnt, 11, lin[2][2:], sts)
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
        ws.write(lcnt, 3, lin[3][2:], sts)
        ws.write(lcnt, 4, sap[lin[0]]["Z2"], sts)
        ws.write(lcnt, 7, int(lin[1]), sts)
        ws.write(lcnt, 8, int(lin[5]), sts)
        ws.write(lcnt, 9, "Cj", sts)
        ws.write(lcnt, 10, lin[2][2:], sts)
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

        lineas = self.srcRep["venta-traslado"]
  
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
                    lin[3] = lin[3].replace("T1632", "T163_2").replace("P60472", "60472")
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
                        for lin in self.srcRep["transportes-cabecera"]:
                            self.imprimirLineaCabecera(lcnt, sap, lin, sts, ws, stsDate)
                            lcnt += 1
                    else:
                        for lin in self.srcRep["transportes-detalle"]:
                            self.imprimirLineaDetalle(lcnt, sap, lin, sts, ws, stsDate, stsTime)
                            lcnt += 1
                    
            wb.save(self.pathSap + "/" + cab + "_" + self.fecha.strftime('%d.%m.%Y') + ".xls")
            
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

    def convertirXLSaXLSX(self, xls, xlsx):
        pythoncom.CoInitialize()
        excel = win32.gencache.EnsureDispatch('Excel.Application') # Calls Excel
        excel.DisplayAlerts = False # Disables prompts, such as asking to overwrite files
        wb = excel.Workbooks.Open(xls.replace('/', '\\')) # Input File
        wb.SaveAs(xlsx.replace('/', '\\'), FileFormat = 51) # Output File
        wb.Close()
        excel.Application.Quit() # Close Excel
        excel.DisplayAlerts = True # Turn alerts back on  
        os.remove(xls) 

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
            ws.write(lcnt, 6, vr[2:], sts)
        else:
            ws.write(lcnt, 6, remplazoPlanta[2:], sts)
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
        for csl in self.srcRep["transportes-detalle"]:
            llegadasCamiones[csl[0]] = csl[2]
            if csl[1].startswith("Pl"):
                llegadasCamionesPL[csl[0]] = [csl[2], csl[3]]
        lcnt = 1
        llaves = set()
        uAndenes = []
        for csl in self.srcRep["capacidades"]:
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
                    self.imprimirLineaZlogTransportes(ws, sts, stsDate, stsTime, stsNum, llegadasCamiones, csl, splr, valn, vr, lcnt, splr[0], splr[0].startswith("Pl") and not splr[1].startswith("Pl")  and not splr[2].startswith("Pl"), llegadasCamionesPL)
                    lcnt += 1

        wb.save(self.pathSap + "/ZLOG_Transportes.xls")
        self.convertirXLSaXLSX(self.pathSap + "/ZLOG_Transportes.xls", self.pathSap + "/ZLOG_Transportes.xlsx")


    def grabarIndicadores(self):
        print("-Grabando Indicadores-")
        sts = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;')
        stsNum = xlwt.easyxf('font: colour black; borders: left thin, right thin, top thin, bottom thin;', num_format_str = '#,##0.000')
        stsb = xlwt.easyxf('font: bold true, colour black; borders: left thin, right thin, top thin, bottom thin;')
        wb = xlwt.Workbook()

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
        
        lcnt = 1
        llaves = set()
        uAndenes = []
        for lidx in self.srcRep["capacidades"]:
            csl = lidx#.split(";")[0:-1]
            #if lcnt > 0:
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
            ws.write(lcnt, 6, vr[2:], sts)
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
                ws.write(lcnt, 6, lin[6][2:], sts)
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
        wb.save(self.pathSap + "/indicadores.xls")
