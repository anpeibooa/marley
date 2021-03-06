#include "marley/marley_utils.hh"
#include "marley/BackshiftedFermiGasModel.hh"
#include "marley/KoningDelarocheOpticalModel.hh"
#include "marley/Logger.hh"
#include "marley/StandardLorentzianModel.hh"
#include "marley/StructureDatabase.hh"

marley::StructureDatabase::StructureDatabase() {}

void marley::StructureDatabase::add_decay_scheme(int pdg,
  std::unique_ptr<marley::DecayScheme>& ds)
{
  auto* temp_ptr = ds.release();
  decay_scheme_table_.emplace(pdg, std::unique_ptr<marley::DecayScheme>(temp_ptr));
}

void marley::StructureDatabase::emplace_decay_scheme(int pdg,
  const std::string& filename, DecayScheme::FileFormat format)
{
  int Z_ds = (pdg % 10000000)/10000;
  int A_ds = (pdg % 10000)/10;

  // Remove the previous entry (if one exists) for the given PDG code
  decay_scheme_table_.erase(pdg);

  // Add the new entry
  decay_scheme_table_.emplace(pdg, std::make_unique<marley::DecayScheme>(
    Z_ds, A_ds, filename, format));
}

std::set<int> marley::StructureDatabase::find_all_nuclides(
  const std::string& filename, DecayScheme::FileFormat format)
{
  if (format != DecayScheme::FileFormat::talys)
    throw marley::Error(std::string("StructureDatabase::")
      + "find_all_nuclides() is not implemented for"
      + " formats other than TALYS.");

  // First line in a TALYS level dataset has fortran
  // format (2i4, 2i5, 56x, i4, a2)
  // General regex for this line:
  static const std::regex nuclide_line("[0-9 ]{18} {56}[0-9 ]{4}.{2}");

  // Open the TALYs level data file for parsing
  std::ifstream file_in(filename);

  // If the file doesn't exist or some other error
  // occurred, complain and give up.
  if (!file_in.good()) throw marley::Error(std::string("Could not")
    + " read from the TALYS data file " + filename);

  std::string line; // String to store the current line
                    // of the TALYS file during parsing

  std::istringstream iss; // String stream used to parse the line

  double Z; // atomic number
  double A; // mass number

  // Particle Data Group codes for each nuclide in the file
  std::set<int> PDGs;

  // Loop through the data file, recording all nuclide PDGs found
  while (std::getline(file_in, line)) {
    if (std::regex_match(line, nuclide_line)) {
      // Load the new line into our istringstream object for parsing. Reset
      // the stream so that we start parsing from the beginning of the string.
      iss.str(line);
      iss.clear();

      // The first two entries on a TALYS nuclide line are Z and A.
      iss >> Z >> A;

      PDGs.insert(marley_utils::get_nucleus_pid(Z, A));
    }
  }

  file_in.close();

  return PDGs;
}

marley::DecayScheme* marley::StructureDatabase::get_decay_scheme(
  const int particle_id)
{
  auto iter = decay_scheme_table_.find(particle_id);
  if (iter == decay_scheme_table_.end()) return nullptr;
  else return iter->second.get();
}

marley::DecayScheme* marley::StructureDatabase::get_decay_scheme(const int Z,
  const int A)
{
  int particle_id = marley_utils::get_nucleus_pid(Z, A);
  return get_decay_scheme(particle_id);
}

marley::OpticalModel& marley::StructureDatabase::get_optical_model(
  int nucleus_pid)
{
  /// @todo add check for invalid nucleus particle ID value
  auto iter = optical_model_table_.find(nucleus_pid);

  if (iter == optical_model_table_.end()) {
    // The requested level density model wasn't found, so create it and add it
    // to the table, returning a reference to the stored level density model
    // afterwards.
    int Z = marley_utils::get_particle_Z(nucleus_pid);
    int A = marley_utils::get_particle_A(nucleus_pid);
    return *(optical_model_table_.emplace(nucleus_pid,
      std::make_unique<marley::KoningDelarocheOpticalModel>(Z, A)).first
      ->second.get());
  }
  else return *(iter->second.get());
}

marley::OpticalModel& marley::StructureDatabase::get_optical_model(
  const int Z, const int A)
{
  int nucleus_pid = marley_utils::get_nucleus_pid(Z, A);
  auto iter = optical_model_table_.find(nucleus_pid);

  if (iter == optical_model_table_.end()) {
    // The requested level density model wasn't found, so create it and add it
    // to the table, returning a reference to the stored level density model
    // afterwards.
    return *(optical_model_table_.emplace(nucleus_pid,
      std::make_unique<marley::KoningDelarocheOpticalModel>(Z, A)).first
      ->second.get());
  }
  else return *(iter->second.get());
}

marley::LevelDensityModel& marley::StructureDatabase::get_level_density_model(
  const int Z, const int A)
{
  int pid = marley_utils::get_nucleus_pid(Z, A);

  auto iter = level_density_table_.find(pid);

  if (iter == level_density_table_.end()) {
    // The requested level density model wasn't found, so create it and add it
    // to the table, returning a reference to the stored level density model
    // afterwards.
    return *(level_density_table_.emplace(pid,
      std::make_unique<marley::BackshiftedFermiGasModel>(Z, A)).first
      ->second.get());
  }
  else return *(iter->second.get());
}

marley::GammaStrengthFunctionModel&
  marley::StructureDatabase::get_gamma_strength_function_model(const int Z,
  const int A)
{
  int pid = marley_utils::get_nucleus_pid(Z, A);

  auto iter = gamma_strength_function_table_.find(pid);

  if (iter == gamma_strength_function_table_.end()) {
    // The requested gamma-ray strength function model wasn't found, so create
    // it and add it to the table, returning a reference to the stored strength
    // function model afterwards.
    return *(gamma_strength_function_table_.emplace(pid,
      std::make_unique<marley::StandardLorentzianModel>(Z, A)).first
      ->second.get());
  }
  else return *(iter->second.get());
}

void marley::StructureDatabase::remove_decay_scheme(int pdg)
{
  // Remove the decay scheme with this PDG code if it exists in the database.
  // If it doesn't, do nothing.
  decay_scheme_table_.erase(pdg);
}

void marley::StructureDatabase::clear() {
  decay_scheme_table_.clear();
}
