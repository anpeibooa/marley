#pragma once
#include <memory>
#include <random>
#include <sstream>
#include <vector>

#include "marley/ConfigurationFile.hh"
#include "marley/GammaStrengthFunctionModel.hh"
#include "marley/NeutrinoSource.hh"
#include "marley/NuclearReaction.hh"
#include "marley/LevelDensityModel.hh"
#include "marley/OpticalModel.hh"
#include "marley/Parity.hh"
#include "marley/RotationMatrix.hh"
#include "marley/StructureDatabase.hh"

namespace marley {

  /// @brief The MARLEY Event generator
  class Generator {

    public:

      /// @brief Create a Generator using default settings
      Generator();

      /// @brief Create a Generator using default settings except for
      /// a given initial seed
      /// @param seed The initial seed to use for this Generator
      Generator(uint_fast64_t seed);

      /// @brief Create a Generator using a ConfigurationFile object
      Generator(marley::ConfigurationFile& cf);

      /// @brief Create a Generator using a configuration file
      /// @param filename Name of the configuration file
      Generator(const std::string& filename);

      /// @brief Create an Event using the NeutrinoSource, Reaction, and
      /// StructureDatabase objects owned by this Generator
      marley::Event create_event();

      /// @brief Get the seed used to initialize this Generator
      inline uint_fast64_t get_seed() const;

      /// @brief Reseeds the Generator
      void reseed(uint_fast64_t seed);

      /// @brief Use a string to set this Generator's internal state
      /// @details This function is typically used to restore a Generator
      /// to a state saved using get_state_string().
      void seed_using_state_string(const std::string& state_string);

      /// @brief Get a string that represents the current internal state of
      /// this Generator
      std::string get_state_string() const;

      /// @brief Sample a random number uniformly on either [min, max) or
      /// [min, max]
      /// @param min Lower bound of the sampling interval
      /// @param max Upper bound of the sampling interval
      /// @param inclusive Whether the upper bound should be included (true)
      /// or excluded (false) from the possible sampling outcomes
      double uniform_random_double(double min, double max, bool inclusive);

      /// @brief Sample from a given 1D probability density function f(x) on
      /// the interval [xmin, xmax] using a simple rejection method
      /// @param f Probability density function to use for sampling
      /// @param xmin Lower bound of the sampling interval
      /// @param xmax Upper bound of the sampling interval
      /// @param max_search_tolerance Tolerance to use when finding the
      /// maximum of f(x) using <a href="http://tinyurl.com/ntqkfck">Brent's
      /// method</a>
      /// @return Sampled value of x
      double rejection_sample(std::function<double(double)> f, double xmin,
        double xmax, double max_search_tolerance
        = DEFAULT_REJECTION_SAMPLING_TOLERANCE_);

      /// @brief Get a reference to the StructureDatabase owned by this
      /// Generator
      marley::StructureDatabase& get_structure_db();

      /// @brief Get a const reference to the vector of Reaction objects
      /// owned by this Generator
      inline const std::vector<std::unique_ptr<marley::Reaction> >&
        get_reactions() const;

      /// @brief Take ownership of a new Reaction
      /// @param reaction A pointer to the new Reaction to use
      void add_reaction(std::unique_ptr<marley::Reaction> reaction);

      /// @brief Clear the vector of Reaction objects owned by this Generator
      void clear_reactions();

      /// @brief Sample a Reaction and an energy for the reacting neutrino
      /// @param[out] E Total energy of the neutrino undergoing the reaction
      /// @return Reference to the sampled Reaction owned by this Generator
      marley::Reaction& sample_reaction(double& E);

      /// @brief Probability density function that describes the distribution
      /// of reacting neutrino energies
      /// @details This function computes the cross-section weighted neutrino
      /// flux (normalized to unity between source_->E_min and
      /// source_->E_max) including cross-section contributions from all
      /// Reactions owned by this Generator. For the distribution of
      /// <i>incident</i> neutrino energies, use marley::NeutrinoSource::pdf()
      /// @param E Total energy of the reacting neutrino
      /// @return Probability density (MeV<sup> -1</sup>)
      double E_pdf(double E);

      /// @brief Get a const reference to the NeutrinoSource owned by this
      /// Generator
      /// @details Throws a marley::Error if this Generator does not own a
      /// NeutrinoSource object.
      const marley::NeutrinoSource& get_source();

      /// @brief Take ownership of a new NeutrinoSource, replacing any
      /// existing source owned by this Generator
      /// @param source A pointer to the new NeutrinoSource to use
      void set_source(std::unique_ptr<marley::NeutrinoSource> source);

      /// @brief Sample from an arbitrary probability distribution (defined
      /// here as any object that implements an operator()(std::mt19937_64&)
      /// function)
      /// @detail This template function
      //  Based on a trick given at https://stackoverflow.com/a/9154394/4081973
      template <class RandomNumberDistribution>
        inline auto sample_from_distribution(RandomNumberDistribution& rnd)
        -> decltype( std::declval<RandomNumberDistribution&>().operator()(
        std::declval<std::mt19937_64&>()) )
      {
        return rnd(rand_gen_);
      }

      /// @brief Sample from an arbitrary probability distribution (defined
      /// here as any object that implements an
      /// operator()(std::mt19937_64&, const ParamType&) function) using the
      /// parameters params
      template <class RandomNumberDistribution, typename ParamType>
        inline auto sample_from_distribution(RandomNumberDistribution& rnd,
        const ParamType& params) -> decltype(
        std::declval<RandomNumberDistribution&>().operator()(
        std::declval<std::mt19937_64&>(), std::declval<const ParamType&>() ) )
      {
        return rnd(rand_gen_, params);
      }

      /// @brief Sets the direction of the incident neutrinos to use when
      /// generating events
      /// @param dir_vec Vector that points in the direction of the incident
      /// neutrinos
      /// @note The dir_vec passed to this function does not need to be
      /// normalized, but it must have at least one nonzero element or a
      /// marley::Error will be thrown.
      void set_neutrino_direction(const std::array<double, 3> dir_vec);

      /// @brief Gets the direction of the incident neutrinos that is used when
      /// generating events
      inline const std::array<double, 3>& neutrino_direction();

      /// @brief Sets the value of the weight_flux flag
      /// @details This is potentially dangerous. Use only if you know
      /// what you are doing.
      void set_weight_flux(bool should_we_weight );

    private:

      /// @brief Helper function that contains initialization code shared by
      /// multiple constructors
      void init(marley::ConfigurationFile& cf);

      /// @brief Helper function that updates the normalization factor to
      /// use in E_pdf()
      void normalize_E_pdf();

      /// @brief Rotates all Particle 3-vectors in a generated Event based
      /// on the incident neutrino direction
      /// @details All Reaction objects currently generate events assuming that
      /// the incident neutrino travels in the +z direction in the lab frame.
      /// The Generator then adjusts this (if needed) using a rotation matrix
      /// just before the finished event is returned by create_event().
      /// @param[in,out] ev Event whose Particle 3-vectors will be rotated
      void rotate_event(marley::Event& ev);

      /// @brief Print the MARLEY logo (called once during construction of
      /// the first Generator object) to any active Logger streams
      void print_logo();

      /// @brief Seed for the random number generator
      uint_fast64_t seed_;

      /// @brief 64-bit Mersenne Twister random number generator
      std::mt19937_64 rand_gen_;

      /// @brief Default stopping tolerance for rejection sampling
      static constexpr double DEFAULT_REJECTION_SAMPLING_TOLERANCE_ = 1e-8;

      /// @brief Normalizaton factor for E_pdf()
      double norm_ = 1.;

      /// @brief NeutrinoSource used to sample reacting neutrino energies
      std::unique_ptr<marley::NeutrinoSource> source_;

      /// @brief StructureDatabase used to simulate nuclear de-excitations
      /// when creating Event objects
      std::unique_ptr<marley::StructureDatabase> structure_db_;

      /// @brief Reaction(s) used to sample reacting neutrino energies
      std::vector<std::unique_ptr<marley::Reaction> > reactions_;

      /// @brief Total cross section values to use as weights for sampling a
      /// Reaction in sample_reaction()
      /// @details These total cross section values are energy dependent. They
      /// are therefore updated with every call to E_pdf().
      std::vector<double> total_xs_values_;

      /// @brief Discrete distribution used for Reaction sampling
      std::discrete_distribution<size_t> r_index_dist_;

      /// @brief Rotation matrix used to update events based on the current
      /// incident neutrino direction
      marley::RotationMatrix rotation_matrix_;

      /// @brief Three-vector that points in the direction of the incident
      /// neutrinos
      std::array<double, 3> dir_vec_;

      /// @brief Boolean value that indicates whether Event objects need
      /// to be rotated (true) or not (false)
      bool need_to_rotate_events_ = false;

      /// @brief Whether the generator should weight the incident
      /// neutrino spectrum by the reaction cross section(s)
      /// @details Don't change this unless you understand what you
      /// are doing!
      bool weight_flux_ = true;
  };

  // Inline function definitions
  inline uint_fast64_t Generator::get_seed() const { return seed_; }

  inline const std::vector<std::unique_ptr<marley::Reaction> >&
    Generator::get_reactions() const { return reactions_; }

  inline const std::array<double, 3>& Generator::neutrino_direction()
    { return dir_vec_; }
}
