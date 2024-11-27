class ExportCPP:
	def __init__(self):
		self.catalogo = {
			"dataStock":[5, 6, 7, 8, 9, 10, 11], 
			"dataDemanda":[2, 4, 5, 6], 
			"dataMateriales":[0, 3, 18, 19, 20], 
			"dataFamilias":[0, 1, 2, 3], 
			"dataCajasPallet":[0, 2], 
			"dataCostoProduccion":[1, 3], 
			"dataPreciosVenta":[1, 3, 5], 
			"dataCV":[0, 2, 4, 5], 
			"dataTransportes":[0, 1, 2, 3, 4], 
			"dataNodos":[0, 1, 6, 7, 8, 9, 10, 11, 12], 
			"dataTViajes":[0, 1, 2] 
		}
		self.almacen = {
			"dataStock":[], 
			"dataDemanda":[], 
			"dataMateriales":[], 
			"dataFamilias":[], 
			"dataCajasPallet":[], 
			"dataCostoProduccion":[], 
			"dataPreciosVenta":[], 
			"dataCV":[], 
			"dataTransportes":[], 
			"dataNodos":[], 
			"dataTViajes":[] 		
		}

	def agregarFila(self, llave, sh, fila, formateador=None, columnaAFormatear=None):
		lFila = []
		for pos in self.catalogo[llave]:
			val = sh.cell_value(rowx=fila, colx=pos)
			if not columnaAFormatear is None and pos == columnaAFormatear:
				val = formateador(val)
			else:
				if isinstance(val, float) and int(val) == val:
					val = int(val)
			lFila.append(val)
		self.almacen[llave].append(lFila)

	def grabarAlmacen(self, nombreArchivo):
		with open(nombreArchivo, 'w', encoding='utf-8') as f:
			for k in self.almacen.keys():
				for l in self.almacen[k]:
					linea = k + ':'
					for e in l:
						linea += (str(e) + ';')
					linea += '\n'
					f.write(linea)

