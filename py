import xlwings as xw
import os
import pythoncom

def enterpriseExcelWrite(enterprise):
    pythoncom.CoInitialize()
    app=xw.App(visible=False,add_book=False)  # visible是否打开文件
    app.display_alerts=False
    app.screen_updating=False
    exceldir = os.path.join('C:\\文件库', 'Projects', 'P' + str(ID))
    excelname = os.path.join(exceldir, 'P' + str(enterprise.enterpriseId) + ".xlsm")
	print(excelname)
    wb=app.books.open(excelname)
    sheet=wb.sheets['信息']
    sheet.range('B1').value = enterprise.enterpriseName
    sheet.range('B2').value = enterprise.nameAbbreviation
    sheet.range('B3').value = enterprise.NEIType
    sheet.range('B4').value = enterprise.environmentAssessmentCompany
    sheet.range('B5').value = enterprise.corporateName
    sheet.range('B6').value = enterprise.corporateId
    sheet.range('B7').value = enterprise.contacts
    sheet.range('B8').value = enterprise.telephone
    sheet.range('B9').value = enterprise.postalCode
    sheet.range('B10').value = enterprise.address
    sheet.range('B11').value = enterprise.totalInvestment
    sheet.range('B12').value = enterprise.environmentalProtectionInvestment
    sheet.range('B13').value = enterprise.environmentalProtectionInvestmentProportion
    sheet.range('B14').value = enterprise.floorSpace
    sheet.range('B15').value = enterprise.managementSpace
    sheet.range('B16').value = enterprise.nonAccommodationNum
    sheet.range('B17').value = enterprise.accommodationNum
    sheet.range('B19').value = enterprise.dayWorkTime
    sheet.range('B20').value = enterprise.yearWorkTime
    sheet.range('B21').value = enterprise.investmentTime
    sheet.range('B22').value = enterprise.productNames
    sheet.range('B23').value = enterprise.constructionScale
    sheet.range('B24').value = enterprise.noiseEquipment
    sheet.range('B25').value = enterprise.noiseMonitoringPoints
    sheet.range('B26').value = enterprise.annualSolidWasteOutput
    sheet.range('B27').value = enterprise.annualPowerConsumption
    sheet.range('B28').value = enterprise.latitude
    sheet.range('B29').value = enterprise.longtitude
    sheet.range('B30').value = enterprise.east
    sheet.range('B31').value = enterprise.south
    sheet.range('B32').value = enterprise.west
    sheet.range('B33').value = enterprise.north
    sheet.range('B34').value = enterprise.township
    sheet.range('B35').value = enterprise.soundEnvironmentStandard
    sheet.range('B36').value = enterprise.groundwaterArea
    sheet.range('B40').value = enterprise.specialOptionforDaliang
    sheet.range('B41').value = enterprise.besideWaterTreatmentPlant
    sheet.range('B48').value = enterprise.sensitivePointDistance
    sheet.range('B49').value = enterprise.waterSourceDistance
    wb.save()
    wb.close()
    app.quit()


def equipmentExcelWrite(set,ID):
    pythoncom.CoInitialize()
    app = xw.App(visible=False, add_book=False)  # visible是否打开文件
    app.display_alerts = False
    app.screen_updating = False
    exceldir = os.path.join('C:\\文件库', 'Projects', 'P' + str(ID))
    excelname = os.path.join(exceldir, 'P' + str(ID) + ".xlsm")
    wb = app.books.open(excelname)
    sheet = wb.sheets['设备']
    num=2
    for f in set:
        equipment = f.save(commit=False)
        list=[equipment.equipName,equipment.num,equipment.unit,equipment.remark]
        print(list)
        row='A'+str(num)
        num+=1
        print(row)
        sheet.range(row).value=list
    wb.save()
    wb.close()
    app.quit()


def materialExcelWrite(set,ID):
    pythoncom.CoInitialize()
    app = xw.App(visible=False, add_book=False)  # visible是否打开文件
    app.display_alerts = False
    app.screen_updating = False
    exceldir = os.path.join('C:\\文件库', 'Projects', 'P' + str(ID))
    excelname = os.path.join(exceldir, 'P' + str(ID) + ".xlsm")
    wb = app.books.open(excelname)
    sheet = wb.sheets['材料']
    num=2
    for f in set:
        material = f.save(commit=False)
        list=[material.materialName,material.num,material.unit,material.isOffcut,material.state]
        print(list)
        row='A'+str(num)
        num+=1
        print(row)
        sheet.range(row).value=list
    wb.save()
    wb.close()
    app.quit()


def productExcelWrite(set,ID):
    pythoncom.CoInitialize()
    app = xw.App(visible=False, add_book=False)  # visible是否打开文件
    app.display_alerts = False
    app.screen_updating = False
    exceldir = os.path.join('C:\\文件库', 'Projects', 'P' + str(ID))
    excelname = os.path.join(exceldir, 'P' + str(ID) + ".xlsm")
    wb = app.books.open(excelname)
    sheet = wb.sheets['产品']
    num=2
    for f in set:
        product = f.save(commit=False)
        list=[product.productsName,product.num,product.unit,product.remark]
        print(list)
        row='A'+str(num)
        num+=1
        print(row)
        sheet.range(row).value=list
    wb.save()
    wb.close()
    app.quit()

def Excelcreate(ID):
    exceldir = os.path.join('C:\\文件库', 'Projects', 'P' + str(ID))
    excelname = os.path.join(exceldir, 'P'+str(ID)+".xlsm")
    if not os.path.isdir(exceldir):
        print("Exceldir creating..."+exceldir)
        os.makedirs(exceldir)
	if not os.path.isfile(excelname)
	    print("Excelfile creating..."+exceldir)
        pythoncom.CoInitialize()
        app = xw.App(visible=False, add_book=False)  # visible是否打开文件
        app.display_alerts = False
        app.screen_updating = False
        modeldir = os.path.join('C:\\文件库', '模板')
        modelpath = os.path.join(modeldir, 'model.xlsm')
        wb = app.books.open(modelpath)
        wb.save(excelname)
        wb.close()
        app.quit()
