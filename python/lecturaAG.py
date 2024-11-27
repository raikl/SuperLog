# -*- coding: utf-8 -*-
import xlrd

def leerXLS(archivo, codigo, prodiedadesHojas, funcProgreso):
    rtn = {}
    funcProgreso(10)
    book = xlrd.open_workbook(archivo)
    for h in prodiedadesHojas: 
        hoja = h[0]
        filaInicio = h[1]
        idsColumnas = h[2]
        lst = []
        sh = book.sheet_by_name(hoja)

        for row_idx in range(filaInicio, sh.nrows): 
            fila = []
            for col_idx in idsColumnas:
                if col_idx < sh.ncols:
                    fila.append(sh.cell_value(rowx=row_idx, colx=col_idx))
            lst.append(fila)
            funcProgreso(50+50*row_idx/sh.nrows)
        rtn[codigo + ":" + hoja] = lst
    return rtn
    
def extraerColumna(matriz, idColumna):
    ls = []
    for i in range(0, len(matriz)):
        ls.append(matriz[i][idColumna])
    return ls

def extraerFilaDesde(matriz, idFila, desde):
    ls = []
    for i in range(desde, len(matriz[idFila])):
        try:
            paso = float(matriz[idFila][i])
            ls.append(matriz[idFila][i])
        except:
            ls.append(None)
    return ls

def leerCSV(archivo, funcProgreso):
    lineas = []
    cnt = 0
    arch = open(archivo, mode="r", encoding="utf-8") 
    aLineas = arch.readlines()
    totalLineas = len(aLineas)
    for lidx in aLineas:
        if cnt > 0:
            lineas.append(lidx.split(";")[:-1])
            funcProgreso(100*cnt/totalLineas)
        cnt += 1
    return lineas

def leerCSV2(archivo, funcProgreso):
    lineas = []
    cnt = 0
    arch = open(archivo, mode="r", encoding="utf-8") 
    aLineas = arch.readlines()
    totalLineas = len(aLineas)
    for lidx in aLineas:
        lineas.append(lidx.split(";")[:-1])
        funcProgreso(100*cnt/totalLineas)
        cnt += 1
    return lineas