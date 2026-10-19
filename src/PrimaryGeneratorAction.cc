
#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
//for randomised gamma rays in a specific direction...\/
#include "Randomize.hh"
#include "G4PhysicalConstants.hh"
#include <cmath>

PrimaryGeneratorAction::PrimaryGeneratorAction() //CREATE GUN?
{
    fParticleGun = new G4ParticleGun(1);

    fParticleGun->SetParticleDefinition(G4Gamma::GammaDefinition());
    //          CHOOSE GAMMA as the particle /\ and /\ defines gamma...
    //                            ...based on what GEANT4 defines it as
    fParticleGun->SetParticleEnergy(1*MeV);               //------ENERGY OF THE GAMMA RAY SHOT - edit here to CHANGE (1MeV = 1000keV)
    //SETS THE GAMMA'S ENERGY
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, -70*cm));
    // This sets the firing position
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    // Makes gamma travel along defined axis as per (x, y, z) values 
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() //DESTROY GUN?
{
    delete fParticleGun;       //deletes the gun 
}

//=============================GUN POSITIONING AND RANDOMIZING========================
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    // Source square: 16 x 16 cm, centred on the z-axis.
    const G4double squareHalfWidth = 8*cm;         //WIDTH (HALF)
    const G4double sourceZ = -70*cm;               //POSITION

    // Independently sample x and y across the square.
    const G4double x =
        (2.0*G4UniformRand() - 1.0)*squareHalfWidth;

    const G4double y =
        (2.0*G4UniformRand() - 1.0)*squareHalfWidth;

    fParticleGun->SetParticlePosition(
        G4ThreeVector(x, y, sourceZ));

    // Random direction within a cone around +z.
    const G4double halfAngle = 5*deg;
    const G4double cosTheta =
        1.0 - G4UniformRand()*(1.0 - std::cos(halfAngle));
    const G4double sinTheta =
        std::sqrt(1.0 - cosTheta*cosTheta);

    const G4double phi = twopi*G4UniformRand();

    const G4ThreeVector direction(
        sinTheta*std::cos(phi),
        sinTheta*std::sin(phi),
        cosTheta
    );

    fParticleGun->SetParticleMomentumDirection(direction);
    fParticleGun->GeneratePrimaryVertex(event);
}
