#include "loadclas.hpp"

using namespace ESM;

const Class::Specialization Class::specializationIds[3] = {
  Class::Combat,
  Class::Magic,
  Class::Stealth
};

const char *Class::gmstSpecializationIds[3] = {
  "sSpecializationCombat",
  "sSpecializationMagic",
  "sSpecializationStealth"
};
