#include <string>
#include <vector>
#include <regex>
#include <iostream> // For testing
#include "marley_utils.hh"
#include "TMarleyLevel.hh"

TMarleyLevel::TMarleyLevel(std::string energy, std::string jpi) {
  sEnergy = energy;
  fEnergy = std::stod(energy); // Converts the energy string into a double

  gammas_known = false; // The default is that gammas are not known
  
  this->set_spin_parity(jpi); // Need to convert jpi to integers in the constructor... Maybe do this a better way
  //spin_parity = jpi;
}

/// Choose a gamma owned by this level randomly based on the relative
/// intensities of all of the gammas.  Return a pointer to the gamma that was
/// chosen.  If this level doesn't have any gammas, return a null pointer.
TMarleyGamma* TMarleyLevel::sample_gamma() {
  if (gammas.empty()) {
    return nullptr;
  }
  else {
    // Get the index of the gamma to return by randomly sampling from the
    // discrete distribution gamma_dist using the standard marley_utils random
    // number generator.
    int g_index = gamma_dist(marley_utils::rand_gen);
    // Return a pointer to the corresponding gamma
    return &(gammas[g_index]);
  }
}

void TMarleyLevel::add_gamma(const TMarleyGamma& gamma) {
  // Update the vector of gamma objects
  gammas.push_back(gamma);

  // Update the vector of gamma intensities
  gamma_intensities.push_back(gamma.get_ri());

  // Update the discrete distribution used to sample gammas
  // when simulating a gamma cascade
  std::discrete_distribution<int>::param_type
    params(gamma_intensities.begin(), gamma_intensities.end());
  gamma_dist.param(params);

  // Update the gamma status to known
    gammas_known = true;

}

double TMarleyLevel::get_numerical_energy() const {
  return fEnergy; 
}

std::string TMarleyLevel::get_string_energy() const {
  return sEnergy; 
}

void TMarleyLevel::set_energy(std::string energy) {
  sEnergy = energy;
  fEnergy = std::stod(energy);
}

std::string TMarleyLevel::get_spin_parity() const {
  return spin_parity;
}

int TMarleyLevel::get_ispin() const
{
  return ispin;
}

int TMarleyLevel::get_iparity() const
{
  return iparity;
}

void TMarleyLevel::set_spin_parity(std::string jpi) {
  spin_parity = jpi;

  // Regular expressions for identifying spin/parity - try to condense if possible
  std::regex jp ("[0-9][+-]"); //For the case jp
  std::regex j1_j2p ("\\([0-9],[0-9]\\)[+-]"); //For the case (j1,j2)p
  std::regex j1p1_j2p2("\\([0-9][+-][,:][0-9][+-]\\)"); // (j1p1,j2p2) or (j1p1:j2p2)
  std::regex jp_par("[0-9]\\([+-]\\)"); // j(p)
  std::regex j1p1_j2p2_nopar("[0-9][+-],[0-9][+-]"); // j1p1,j2p2
  std::regex le_jp("\\(LE [0-9]\\)[+-]"); // (LE j)p
  std::regex j1_j2p2("\\([0-9],[0-9][+-]\\)"); // (j1,j2p2) --------Not behaving correctly
  std::regex jp_inpar("\\([0-9][+-]\\)"); // (jp)

  if(std::regex_match(spin_parity, jp)) // jp
    {
      ispin = std::stoi(spin_parity.substr(0,1));
      
      if(spin_parity.substr(1,2) == "+")
	iparity = 1;
      else
	iparity = -1;
    }

  else if (std::regex_match(spin_parity, j1_j2p)) // (j1,j2)p
    {
      ispin = std::stoi(spin_parity.substr(1,2)); //Picking the first spin only
	  	  
      if(spin_parity.substr(5,6) == "+")
	iparity = 1;
      else
	iparity = -1;
    }

  else if (std::regex_match(spin_parity, j1p1_j2p2)) // (j1p1,j2p2) or (j1p1:j2p2)
    {
      ispin = std::stod(spin_parity.substr(1,2));
	 	  
      if(spin_parity.substr(2,3) == "+")
	iparity = 1;
      else
	iparity = -1;
    }

  else if (std::regex_match(spin_parity, jp_par)) // j(p)
    {
      ispin = std::stoi(spin_parity.substr(0,1));
     
      if(spin_parity.substr(2,3) == "+")
	iparity = 1;
      else
	iparity = -1;
    }

  else if (std::regex_match(spin_parity, jp_inpar)) // (jp)
    {
      ispin = std::stoi(spin_parity.substr(1,2));
      
      if(spin_parity.substr(2,3) == "+")
	iparity = 1;
      else
	iparity = -1;
    }

  else if (std::regex_match(spin_parity, j1p1_j2p2_nopar)) // j1p1,j2p2 (no parentheses)
    {
      ispin = std::stoi(spin_parity.substr(0,1));
	 	  
      if(spin_parity.substr(1,2) == "+")
	iparity = 1;
      else
	iparity = -1;
    }

  else if (std::regex_match(spin_parity, le_jp)) // (LE j)p
    {
      ispin = std::stoi(spin_parity.substr(4,5));
      
      if(spin_parity.substr(6,7) == "+")
	iparity = 1;
      else
	iparity = -1;
    }

  else if (std::regex_match(spin_parity, j1_j2p2)) // (j1,j2p2) //---------------Not working correctly (j,jp)
    {
      ispin = std::stoi(spin_parity.substr(3,4)); //Picking the first spin only

      if(spin_parity.substr(4,5) == "+")
	iparity = 1;
      else
	iparity = -1;
    }
    
  else //Default jp is 1+
    {
      ispin = 1;
      iparity = 1;
    }

  //For testing purposes:
  std::cout << "Spin: " << ispin << " " << "Parity: " << iparity << std::endl;
}

void TMarleyLevel::clear_gammas() {
  gammas.clear();
  gamma_intensities.clear();
  // The discrete distribution will be cleared by these
  // commands because gamma_intensities is now empty
  std::discrete_distribution<int>::param_type
    params(gamma_intensities.begin(), gamma_intensities.end());
  gamma_dist.param(params);
}

std::vector<TMarleyGamma>* TMarleyLevel::get_gammas() {
  return &gammas;
}


bool TMarleyLevel::get_gamma_status() const
{
  return gammas_known;
}
