
#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"

#include "G4Box.hh"
#include "G4SubtractionSolid.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"

#include "G4Cons.hh"
#include "G4Orb.hh"
#include "G4Sphere.hh"
#include "G4Trd.hh"
#include "G4LogicalVolume.hh"

#include "SiliconPixelSD.hh"

#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include <vector>
#include <cstdlib>
#include <string>
#include <cmath>



G4LogicalVolume* HexagonLogical(G4String name, G4double cellThickness, G4double cellSideLength, G4Material* material) {
  G4double full_cellX = sqrt(3.) * cellSideLength;
  G4double full_cellY = (2.) * cellSideLength;
  G4Box* solidFullcell = new G4Box(name,                       //its name
                                   0.5 * full_cellX, 0.5 * full_cellY, 0.5 * cellThickness); //its size


  G4double deltaXDash = cellSideLength;
  G4double deltaYDash = sqrt(3) / 4 * cellSideLength;

  G4Box* solidCutcell = new G4Box(name,                       //its name
                                  0.5 * deltaXDash, 0.5 * (deltaYDash), 1.*cellThickness); //its size


  G4double DeltaTheta[4] = {60.*deg, 120.*deg, 240.*deg, 300.*deg};
  G4double DeltaTheta_rot[4] = {30.*deg, 150.*deg, 210 * deg, 330 * deg};
  G4double Delta = sqrt(3) / 2 * cellSideLength + deltaYDash / 2;

  G4RotationMatrix* rot = new G4RotationMatrix;
  rot->rotateZ(DeltaTheta_rot[0]);
  std::vector<G4SubtractionSolid*> subtracted;
  subtracted.push_back(new G4SubtractionSolid("cellSubtracted", solidFullcell, solidCutcell, rot, G4ThreeVector(cos(DeltaTheta[0])*Delta, sin(DeltaTheta[0])*Delta, 0.)));

  for (int i = 1; i < 4; i++) {
    rot->rotateZ(-DeltaTheta_rot[i - 1]);
    rot->rotateZ(DeltaTheta_rot[i]);
    subtracted.push_back(new G4SubtractionSolid("cellSubtracted", subtracted[i - 1], solidCutcell, rot, G4ThreeVector(cos(DeltaTheta[i])*Delta, sin(DeltaTheta[i])*Delta, 0.)));
  }

  return new G4LogicalVolume(subtracted[3],          //its solid
                             material,           //its material
                             name);            //its name

}



//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::DetectorConstruction()
  : G4VUserDetectorConstruction(),
    fScoringVolume(0),
    _configuration(-1)
{ 
  absPbEE_pre_config101 = 3 * mm;
  absPbEE_post_config101 = 3 * mm;
  DefineCommands(); }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::~DetectorConstruction()
{ }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::Construct()
{

  // visual attributes
  G4VisAttributes* visAttributes;


  /***** Definition of all available materials *****/
  // Get nist material manager
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* mat_AIR = nist->FindOrBuildMaterial("G4_AIR");
  G4Material* mat_Ar = nist->FindOrBuildMaterial("G4_Ar");
  G4Material* mat_Al = nist->FindOrBuildMaterial("G4_Al");
  G4Material* mat_Fe = nist->FindOrBuildMaterial("G4_Fe");
  G4Material* mat_Glass = nist->FindOrBuildMaterial("G4_GLASS_PLATE");
  G4Material* mat_Steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");
  G4Material* mat_Pb = nist->FindOrBuildMaterial("G4_Pb");
  G4Material* mat_Cu = nist->FindOrBuildMaterial("G4_Cu");
  G4Material* mat_W = nist->FindOrBuildMaterial("G4_W");
  G4Material* mat_Si = nist->FindOrBuildMaterial("G4_Si");
  G4Material* mat_KAPTON = nist->FindOrBuildMaterial("G4_KAPTON");
  G4Material* mat_PCB = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
  //CuW alloy: 60% Cu, 40% W in mass
  G4double Cu_frac_in_CuW = 0.6;
  G4Material* mat_CuW = new G4Material("CuW", mat_Cu->GetDensity()*Cu_frac_in_CuW + mat_W->GetDensity() * (1 - Cu_frac_in_CuW), 2);
  mat_CuW->AddMaterial(mat_Cu, Cu_frac_in_CuW);
  mat_CuW->AddMaterial(mat_W, 1 - Cu_frac_in_CuW);


  /***** Definition of the world = beam line *****/
  beamLineLength = 32 * m;
  beamLineXY = 9 * m;

  // World = Beam line
  G4Box* solidWorld = new G4Box("World", 0.5 * beamLineXY, 0.5 * beamLineXY, 0.5 * beamLineLength);

  G4Material* world_mat = mat_AIR;
  logicWorld = new G4LogicalVolume(solidWorld, world_mat, "World");



  /***** Definition of silicon (wafer) sensors *****/
  //300 microns thickness only
  Si_pixel_sideLength = 0.6496345 * cm;
  Si_wafer_thickness = 0.3 * mm;
  alpha = 60. / 180. * M_PI;
  Si_wafer_sideLength = 11 * Si_pixel_sideLength;

  Si_wafer_logical = HexagonLogical("Si_wafer", Si_wafer_thickness, Si_wafer_sideLength, mat_AIR);
  visAttributes = new G4VisAttributes(G4Colour(.0, 0.0, 0.0));
  visAttributes->SetVisibility(false);
  Si_wafer_logical->SetVisAttributes(visAttributes);

  //Silicon pixel setups
  double dx = 2 * sin(alpha) * Si_pixel_sideLength;
  double dy = Si_pixel_sideLength * (2. + 2 * cos(alpha));
  Si_pixel_logical = HexagonLogical("SiCell", Si_wafer_thickness, Si_pixel_sideLength, mat_Si);
  visAttributes = new G4VisAttributes(G4Colour(.3, 0.3, 0.3, 1.0));
  visAttributes->SetVisibility(true);
  Si_pixel_logical->SetVisAttributes(visAttributes);

  new G4PVPlacement(0, G4ThreeVector(0, 0., 0.), Si_pixel_logical, "SiCell", Si_wafer_logical, false, 0, true);
  int index = 1;
  int nRows[11] = {7, 6, 7, 6, 5, 6, 5, 4, 5, 4, 3};
  for (int nC = 0; nC < 11; nC++) {
    for (int middle_index = 0; middle_index < nRows[nC]; middle_index++) {
      new G4PVPlacement(0, G4ThreeVector(nC * dx / 2, dy * (middle_index - nRows[nC] / 2. + 0.5), 0.), Si_pixel_logical, "SiCell", Si_wafer_logical, false, index++, true);
      if (nC <= 0) continue;
      new G4PVPlacement(0, G4ThreeVector(-nC * dx / 2, dy * (middle_index - nRows[nC] / 2. + 0.5), 0.), Si_pixel_logical, "SiCell", Si_wafer_logical, false, index++, true);
    }
  }


  thickness_map["Si_wafer"] = Si_wafer_thickness;
  logical_volume_map["Si_wafer"] = Si_wafer_logical;

  /***** Definition of all baseplates *****/
  //CuW
  G4double CuW_baseplate_thickness = 1.2 * mm;
  G4double CuW_baseplate_sideLength = 11 * Si_pixel_sideLength;
  CuW_baseplate_logical = HexagonLogical("CuW_baseplate", CuW_baseplate_thickness, CuW_baseplate_sideLength, mat_CuW);
  visAttributes = new G4VisAttributes(G4Colour(.5, 0.0, 0.5, 0.3));
  visAttributes->SetVisibility(true);
  CuW_baseplate_logical->SetVisAttributes(visAttributes);
  thickness_map["CuW_baseplate"] = CuW_baseplate_thickness;
  logical_volume_map["CuW_baseplate"] = CuW_baseplate_logical;

  //Cu
  G4double Cu_baseplate_thickness = 1.2 * mm;
  G4double Cu_baseplate_sideLength = 11 * Si_pixel_sideLength;
  Cu_baseplate_logical = HexagonLogical("Cu_baseplate", Cu_baseplate_thickness, Cu_baseplate_sideLength, mat_Cu);
  visAttributes = new G4VisAttributes(G4Colour(.1, 0.2, 0.5, 0.3));
  visAttributes->SetVisibility(true);
  Cu_baseplate_logical->SetVisAttributes(visAttributes);
  thickness_map["Cu_baseplate"] = Cu_baseplate_thickness;
  logical_volume_map["Cu_baseplate"] = Cu_baseplate_logical;

  //PCB
  G4double PCB_baseplate_thickness = 1.2 * mm;
  G4double PCB_baseplate_sideLength = 11 * Si_pixel_sideLength;
  PCB_baseplate_logical = HexagonLogical("PCB", PCB_baseplate_thickness, PCB_baseplate_sideLength, mat_PCB);
  visAttributes = new G4VisAttributes(G4Colour(.0, 1., 0.0, 0.3));
  visAttributes->SetVisibility(true);
  PCB_baseplate_logical->SetVisAttributes(visAttributes);
  thickness_map["PCB"] = PCB_baseplate_thickness;
  logical_volume_map["PCB"] = PCB_baseplate_logical;

  //Kapton layer
  G4double Kapton_layer_thickness = 1.2 * mm;
  G4double Kapton_layer_sideLength = 11 * Si_pixel_sideLength;
  Kapton_layer_logical = HexagonLogical("Kapton_layer", Kapton_layer_thickness, Kapton_layer_sideLength, mat_KAPTON);
  visAttributes = new G4VisAttributes(G4Colour(.4, 0.4, 0.0, 0.3));
  visAttributes->SetVisibility(true);
  Kapton_layer_logical->SetVisAttributes(visAttributes);
  thickness_map["Kapton_layer"] = Kapton_layer_thickness;
  logical_volume_map["Kapton_layer"] = Kapton_layer_logical;



  /***** Definition of absorber plates *****/
  G4double Al_case_thickness = 2.6 * mm;
  G4double Al_case_xy = 60 * cm;
  G4Box* Al_case_solid = new G4Box("Al_case", 0.5 * Al_case_xy, 0.5 * Al_case_xy, 0.5 * Al_case_thickness);
  Al_case_logical = new G4LogicalVolume(Al_case_solid, mat_Al, "Al_case");
  visAttributes = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 0.5));
  visAttributes->SetVisibility(true);
  Al_case_logical->SetVisAttributes(visAttributes);
  thickness_map["Al_case"] = Al_case_thickness;
  logical_volume_map["Al_case"] = Al_case_logical;

  G4double Steel_case_thickness = 0.9 * cm;
  G4double Steel_case_xy = 60 * cm;
  G4Box* Steel_case_solid = new G4Box("Steel_case", 0.5 * Steel_case_xy, 0.5 * Steel_case_xy, 0.5 * Steel_case_thickness);
  Steel_case_logical = new G4LogicalVolume(Steel_case_solid, mat_Steel, "Steel_case");
  visAttributes = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 0.5));
  visAttributes->SetVisibility(true);
  Steel_case_logical->SetVisAttributes(visAttributes);
  thickness_map["Steel_case"] = Steel_case_thickness;
  logical_volume_map["Steel_case"] = Steel_case_logical;

  //defintion of absorber plates in the EE part
  G4double Pb_absorber_EE_thickness = 4.9 * mm;
  G4double Pb_absorber_EE_xy = 30 * cm;
  G4Box* Pb_absorber_EE_solid = new G4Box("Pb_absorber_EE", 0.5 * Pb_absorber_EE_xy, 0.5 * Pb_absorber_EE_xy, 0.5 * Pb_absorber_EE_thickness);
  Pb_absorber_EE_logical = new G4LogicalVolume(Pb_absorber_EE_solid, mat_Pb, "Pb_absorber_EE");
  visAttributes = new G4VisAttributes(G4Colour(0.1, 0.4, 0.8, 0.1));
  visAttributes->SetVisibility(true);
  Pb_absorber_EE_logical->SetVisAttributes(visAttributes);
  thickness_map["Pb_absorber_EE"] = Pb_absorber_EE_thickness;
  logical_volume_map["Pb_absorber_EE"] = Pb_absorber_EE_logical;

  G4double Cu_absorber_EE_thickness = 6 * mm;
  G4double Cu_absorber_EE_xy = 30 * cm;
  G4Box* Cu_absorber_EE_solid = new G4Box("Cu_absorber_EE", 0.5 * Cu_absorber_EE_xy, 0.5 * Cu_absorber_EE_xy, 0.5 * Cu_absorber_EE_thickness);
  Cu_absorber_EE_logical = new G4LogicalVolume(Cu_absorber_EE_solid, mat_Cu, "Cu_absorber_EE");
  visAttributes = new G4VisAttributes(G4Colour(.1, 0.2, 0.5, 0.1));
  visAttributes->SetVisibility(true);
  Cu_absorber_EE_logical->SetVisAttributes(visAttributes);
  thickness_map["Cu_absorber_EE"] = Cu_absorber_EE_thickness;
  logical_volume_map["Cu_absorber_EE"] = Cu_absorber_EE_logical;

  //defintion of absorber plates in the FH part
  G4double Cu_absorber_FH_thickness = 6 * mm;
  G4double Cu_absorber_FH_xy = 50 * cm;
  G4Box* Cu_absorber_FH_solid = new G4Box("Cu_absorber_FH", 0.5 * Cu_absorber_FH_xy, 0.5 * Cu_absorber_FH_xy, 0.5 * Cu_absorber_FH_thickness);
  Cu_absorber_FH_logical = new G4LogicalVolume(Cu_absorber_FH_solid, mat_Cu, "Cu_absorber_FH");
  visAttributes = new G4VisAttributes(G4Colour(.1, 0.2, 0.5, 0.1));
  visAttributes->SetVisibility(true);
  Cu_absorber_FH_logical->SetVisAttributes(visAttributes);
  thickness_map["Cu_absorber_FH"] = Cu_absorber_FH_thickness;
  logical_volume_map["Cu_absorber_FH"] = Cu_absorber_FH_logical;

  G4double Fe_absorber_FH_thickness = 41 * mm;
  G4double Fe_absorber_FH_xy = 50 * cm;
  G4Box* Fe_absorber_FH_solid = new G4Box("Fe_absorber_FH", 0.5 * Fe_absorber_FH_xy, 0.5 * Fe_absorber_FH_xy, 0.5 * Fe_absorber_FH_thickness);
  Fe_absorber_FH_logical = new G4LogicalVolume(Fe_absorber_FH_solid, mat_Fe, "Fe_absorber_FH");
  visAttributes = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 0.1));
  visAttributes->SetVisibility(true);
  Fe_absorber_FH_logical->SetVisAttributes(visAttributes);
  thickness_map["Fe_absorber_FH"] = Fe_absorber_FH_thickness;
  logical_volume_map["Fe_absorber_FH"] = Fe_absorber_FH_logical;


  /***** Definition of beam line elements *****/
  //scintillators
  G4double scintillator_thickness = 10 * mm;
  G4double scintillator_xy = 10 * cm;
  G4Box* scintillator_solid = new G4Box("Scintillator", 0.5 * scintillator_xy, 0.5 * scintillator_xy, 0.5 * scintillator_thickness);
  scintillator_logical = new G4LogicalVolume(scintillator_solid, mat_PCB, "Scintillator");
  visAttributes = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.5));
  visAttributes->SetVisibility(true);
  scintillator_logical->SetVisAttributes(visAttributes);
  thickness_map["Scintillator"] = scintillator_thickness;
  logical_volume_map["Scintillator"] = scintillator_logical;


  //DWC related material
  G4double DWC_thickness = 10 * mm;
  G4double DWC_xy = 10 * cm;
  G4Box* DWC_solid = new G4Box("DWC", 0.5 * DWC_xy, 0.5 * DWC_xy, 0.5 * DWC_thickness);
  DWC_logical = new G4LogicalVolume(DWC_solid, mat_Glass, "DWC");
  visAttributes = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.7));
  visAttributes->SetVisibility(true);
  DWC_logical->SetVisAttributes(visAttributes);
  thickness_map["DWC"] = DWC_thickness;
  logical_volume_map["DWC"] = DWC_logical;

  G4double DWC_gas_thickness = DWC_thickness - 2 * mm;
  G4double DWC_gas_xy = DWC_xy - 2 * mm;
  G4Box * DWC_gas_solid = new G4Box("DWC_gas", 0.5 * DWC_gas_xy, 0.5 * DWC_gas_xy, 0.5 * DWC_gas_thickness);
  DWC_gas_logical = new G4LogicalVolume(DWC_gas_solid, mat_Ar, "DWC_gas");
  visAttributes = new G4VisAttributes(G4Colour(0.05, 0.05, 0.05, 0.0));
  visAttributes->SetVisibility(true);
  DWC_gas_logical->SetVisAttributes(visAttributes);


  new G4PVPlacement(0,                     //no rotation
                    G4ThreeVector(0, 0., 0.),       //at (0,0,0)
                    DWC_gas_logical,            //its logical volume
                    "DWC_gas",               //its name
                    DWC_logical,                     //its mother  volume
                    false,                 //no boolean operation
                    0,                     //copy number
                    true);        //overlaps checking



  /****  START OF TEST ****/
  G4VPhysicalVolume* physWorld =
    new G4PVPlacement(0,                     //no rotation
                      G4ThreeVector(0., 0., 0.),       //at (0,0,0)
                      logicWorld,            //its logical volume
                      "World",               //its name
                      0,                     //its mother  volume
                      false,                 //no boolean operation
                      0,                     //copy number
                      true);        //overlaps checking


  /****  END OF TEST ****/


  fScoringVolume = logicWorld;


  return physWorld;
}


void DetectorConstruction::ConstructHGCal() {
  /***** Start the placement *****/
  //
  //
  // Option to switch on/off checking of volumes overlaps
  //


  std::vector<std::pair<std::string, G4double> > dz_map;

  G4double z0 = -beamLineLength / 2.;

  if (_configuration == 22) {
    dz_map.push_back(std::make_pair("DWC", 0.0 * m));
    dz_map.push_back(std::make_pair("DWC", 2.0 * m));
    dz_map.push_back(std::make_pair("DWC", 0.3 * m));
    dz_map.push_back(std::make_pair("Scintillator", 1.5 * m));
    dz_map.push_back(std::make_pair("DWC", 0.3 * m));
    dz_map.push_back(std::make_pair("DWC", 15. * m));
    dz_map.push_back(std::make_pair("DWC", 7. * m));

    dz_map.push_back(std::make_pair("Scintillator", 0.3 * m));
    dz_map.push_back(std::make_pair("Scintillator", 2.0 * m));
    
    dz_map.push_back(std::make_pair("Al_case", 0.1 * m));

    //EE1
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 12 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.3 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    dz_map.push_back(std::make_pair("Al_case", 0.6 * cm));

    //EE2
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 0.5 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE3
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.1 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE4
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.2 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE5
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.2 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE6
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.2 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE7
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.0 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE8
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.0 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE9
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.0 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE10
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.0 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE11
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.0 * cm));
    dz_map.push_back(std::make_pair("PCB", 1.0 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE12
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.4 * cm));
    dz_map.push_back(std::make_pair("PCB", 1.0 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE13
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.4 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    //EE14
    dz_map.push_back(std::make_pair("Pb_absorber_EE", 1.4 * cm));
    dz_map.push_back(std::make_pair("PCB", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));

    dz_map.push_back(std::make_pair("Al_case", 8.4 * cm));

    //beginning of FH
    dz_map.push_back(std::make_pair("Steel_case", 3.5 * cm));

    //FH6, orientation correct?
    dz_map.push_back(std::make_pair("PCB_DAISY", 0.3 * cm));
    dz_map.push_back(std::make_pair("Si_wafer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 1.8 * cm));

    //FH3, orientation correct?
    dz_map.push_back(std::make_pair("PCB_DAISY", 0.3 * cm));
    dz_map.push_back(std::make_pair("Si_wafer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 1.3 * cm));

    //FH2, orientation correct?
    dz_map.push_back(std::make_pair("PCB_DAISY", 0.8 * cm));
    dz_map.push_back(std::make_pair("Si_wafer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 1.5 * cm));

    //FH5, orientation correct?
    dz_map.push_back(std::make_pair("PCB_DAISY", 0.7 * cm));
    dz_map.push_back(std::make_pair("Si_wafer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 1.7 * cm));

    //FH8, orientation correct?
    dz_map.push_back(std::make_pair("PCB_DAISY", 0.4 * cm));
    dz_map.push_back(std::make_pair("Si_wafer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 1.6 * cm));

    //FH9, orientation correct?
    dz_map.push_back(std::make_pair("PCB_DAISY", 0.5 * cm));
    dz_map.push_back(std::make_pair("Si_wafer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));


    dz_map.push_back(std::make_pair("Steel_case", 1.4 * cm));
    dz_map.push_back(std::make_pair("Fe_absorber_FH", 3.6 * cm));
    dz_map.push_back(std::make_pair("Steel_case", 5.2 * cm));


    //FH7, orientation correct?
    dz_map.push_back(std::make_pair("PCB_DAISY", 0.3 * cm));
    dz_map.push_back(std::make_pair("Si_wafer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 1.7 * cm));

    //FH1, orientation correct?
    dz_map.push_back(std::make_pair("PCB_DAISY", 1.1 * cm));
    dz_map.push_back(std::make_pair("Si_wafer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 1.9 * cm));

    //FH2, orientation correct?
    dz_map.push_back(std::make_pair("PCB_DAISY", 0.9 * cm));
    dz_map.push_back(std::make_pair("Si_wafer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate_DAISY", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 2.0 * cm));

    //FH10, orientation correct?
    dz_map.push_back(std::make_pair("PCB", 1.1 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 1.8 * cm));

    //FH11, orientation correct?
    dz_map.push_back(std::make_pair("PCB", 1.0 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Fe_absorber_FH", 1.7 * cm));

    //FH12, orientation correct?
    dz_map.push_back(std::make_pair("PCB", 1.0 * cm));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Cu_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_FH", 0.));

    dz_map.push_back(std::make_pair("Steel_case", 1.9 * cm));
  }


  if (_configuration == 101) {
    G4double dzPbEE = 0.1 * mm;
    //EE1
    dz_map.push_back(std::make_pair("Pb_absorber_EE", beamLineLength/2 + 13.3 * m));
    dz_map.push_back(std::make_pair("Pb_absorber_EE", dzPbEE));
    dz_map.push_back(std::make_pair("Pb_absorber_EE", dzPbEE));
    dz_map.push_back(std::make_pair("Pb_absorber_EE", dzPbEE));
    dz_map.push_back(std::make_pair("Pb_absorber_EE", dzPbEE));
    
    dz_map.push_back(std::make_pair("PCB", absPbEE_pre_config101));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Cu_absorber_EE", 0.));
    dz_map.push_back(std::make_pair("CuW_baseplate", 0.));
    dz_map.push_back(std::make_pair("Kapton_layer", 0.));
    dz_map.push_back(std::make_pair("Si_wafer", 0.));
    dz_map.push_back(std::make_pair("PCB", 0));
    
    dz_map.push_back(std::make_pair("Pb_absorber_EE", absPbEE_post_config101));
    dz_map.push_back(std::make_pair("Pb_absorber_EE", dzPbEE));
  }



  /*
    case "DAISY_WAFER"
    case "Si_wafer":
    case "CuW_baseplate":
    case "Cu_baseplate":
    case "PCB":
    case "Kapton_layer":
    case "Al_case":
    case "Steel_case":
    case "Pb_absorber_EE":
    case "Cu_absorber_EE":
    case "Cu_absorber_FH":
    case "Fe_absorber_FH":
    case "Scintillator":
    case "DWC":
      break;
    default:
  */


  /*****    START GENERIC PLACEMENT ALGORITHM    *****/
  std::map<std::string, int> copy_counter_map;
  for (size_t item_index = 0; item_index < dz_map.size(); item_index++) {
    std::string item_type = dz_map[item_index].first;
    G4double dz = dz_map[item_index].second;
    z0 += dz;

    std::cout << "Placing " << item_type << " at position z [mm]=" << z0 / mm - 12590 << std::endl;
    if (item_type.find("_DAISY") != std::string::npos) {
      item_type.resize(item_type.find("_DAISY"));
      if (copy_counter_map.find(item_type) == copy_counter_map.end()) copy_counter_map[item_type] = 0;
      double dx_ = 2 * sin(alpha) * 11 * Si_pixel_sideLength;
      double dy_ = 11 * Si_pixel_sideLength * (2. + 2 * cos(alpha));
      int nRows_[3] = {1, 2, 1};
      for (int nC = 0; nC < 3; nC++) {
        for (int middle_index = 0; middle_index < nRows_[nC]; middle_index++) {
          new G4PVPlacement(0, G4ThreeVector(nC * dx_ / 2, dy_ * (middle_index - nRows_[nC] / 2. + 0.5), z0 + 0.5 * thickness_map[item_type]), logical_volume_map[item_type], item_type, logicWorld, false, copy_counter_map[item_type]++, true);
          if (nC <= 0) continue;
          new G4PVPlacement(0, G4ThreeVector(-nC * dx_ / 2, dy_ * (middle_index - nRows_[nC] / 2. + 0.5), z0 + 0.5 * thickness_map[item_type]), logical_volume_map[item_type], item_type, logicWorld, false, copy_counter_map[item_type]++, true);
        }
      }
      z0 += thickness_map[item_type];
    } else {
      if (copy_counter_map.find(item_type) == copy_counter_map.end()) copy_counter_map[item_type] = 0;
      new G4PVPlacement(0, G4ThreeVector(0., 0., z0 + 0.5 * thickness_map[item_type]), logical_volume_map[item_type], item_type, logicWorld, false, copy_counter_map[item_type]++, true); //todo: index
      z0 += thickness_map[item_type];
    }
  }

}

void DetectorConstruction::ConstructSDandField() {
  G4SDManager* sdman = G4SDManager::GetSDMpointer();

  SiliconPixelSD* sensitive = new SiliconPixelSD((Si_pixel_logical->GetName() + "_sensitive").c_str());
  sdman->AddNewDetector(sensitive);
  Si_pixel_logical->SetSensitiveDetector(sensitive);


}

void DetectorConstruction::SelectConfiguration(G4int val) {

  if (_configuration != -1) return;

  _configuration = val;

  ConstructHGCal();
  // tell G4RunManager that we change the geometry
  G4RunManager::GetRunManager()->GeometryHasBeenModified();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::DefineCommands()
{
  // Define /B5/detector command directory using generic messenger class
  fMessenger = new G4GenericMessenger(this,
                                      "/HGCalOctober2018/setup/",
                                      "Configuration specifications");


  absPbEE_pre_config101 = 3 * mm;
  absPbEE_post_config101 = 3 * mm;


  //commands specific to configuration 101
  auto& absPbEE_pre_config101Cmd
      = fMessenger->DeclarePropertyWithUnit("absPbEE_pre_config101", "mm", absPbEE_pre_config101,
              "Distance between upstream Pb absorber and cassette.");
  absPbEE_pre_config101Cmd.SetParameterName("absPbEE_pre_config101", true);
  absPbEE_pre_config101Cmd.SetRange("absPbEE_pre_config101>=0");
  absPbEE_pre_config101Cmd.SetDefaultValue("3");  

  auto& absPbEE_post_config101Cmd
      = fMessenger->DeclarePropertyWithUnit("absPbEE_post_config101", "mm", absPbEE_post_config101,
              "Distance between downstream Pb absorber and cassette.");
  absPbEE_post_config101Cmd.SetParameterName("absPbEE_post_config101", true);
  absPbEE_post_config101Cmd.SetRange("absPbEE_post_config101>=0");
  absPbEE_post_config101Cmd.SetDefaultValue("3");  



  // configuration command 
  auto& configeCmd
    = fMessenger->DeclareMethod("config", 
                                        &DetectorConstruction::SelectConfiguration,
                                        "Select the configuration (22-24)");
  configeCmd.SetParameterName("config", true);
  configeCmd.SetDefaultValue("22");
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
