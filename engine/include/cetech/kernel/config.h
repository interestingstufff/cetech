//! \defgroup Config
//! Config system
//! \{
#ifndef CETECH_CONFIG_H
#define CETECH_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>

struct api_v0;
struct memory_api_v0;
struct app_api_v0;

//==============================================================================
// Typedefs
//==============================================================================

//! Cvar type
typedef struct {
    uint64_t idx; //!< Idx
} cvar_t;


//==============================================================================
// Enums
//==============================================================================

//! Cvar types enum
enum cvar_type {
    CV_NONE = 0, //!< Invalid type
    CV_FLOAT,    //!< Float type
    CV_INT,      //!< Int type
    CV_STRING    //!< String type
};

//==============================================================================
// Api
//==============================================================================

//! Config API V0
struct config_api_v0 {
    void (*load_global)(struct app_api_v0 *app_api);

#ifdef CETECH_CAN_COMPILE

    void (*compile_global)(struct app_api_v0 *app_api);

#endif

    //! Parse commandline arguments.
    //! \param args Arguments
    //! \return 1 if ok else 0
    int (*parse_args)(int argc,
                      const char **argv);

    //! Find cvar
    //! \param name Cvar name
    //! \return Cvar or null cvar if name not found.
    cvar_t (*find)(const char *name);

    //! Find cvar if exist else create new
    //! \param name Cvar name
    //! \param is_new Write 1 if cvar is new
    //! \return Cvar
    cvar_t (*find_or_create)(const char *name,
                             int *is_new);

    //! Create new float cvar
    //! \param name Name
    //! \param desc Description
    //! \param f Value
    //! \return Cvar
    cvar_t (*new_float)(const char *name,
                        const char *desc,
                        float f);

    //! Create new intcvar
    //! \param name Name
    //! \param desc Description
    //! \param i Value
    //! \return Cvar
    cvar_t (*new_int)(const char *name,
                      const char *desc,
                      int i);

    //! Create new string cvar
    //! \param name Name
    //! \param desc Description
    //! \param s Value
    //! \return Cvar
    cvar_t (*new_str)(const char *name,
                      const char *desc,
                      const char *s);

    //! Get float value
    //! \param var Cvar
    //! \return Float value
    float (*get_float)(cvar_t var);

    //! Get int value
    //! \param var Cvar
    //! \return Int value
    int (*get_int)(cvar_t var);

    //! Get string value
    //! \param var Cvar
    //! \return String value
    const char *(*get_string)(cvar_t var);

    //! Get cvar type
    //! \param var Cvar
    //! \return Cvart type
    enum cvar_type (*get_type)(cvar_t var);

    //! Set float value
    //! \param var Cvar
    //! \param f Value
    void (*set_float)(cvar_t var,
                      float f);

    //! Set int value
    //! \param var Cvar
    //! \param i Value
    void (*set_int)(cvar_t var,
                    int i);

    //! Set string value
    //! \param var Cvar
    //! \param s Value
    void (*set_string)(cvar_t var,
                       const char *s);

    //! Dump all variables to log
    void (*log_all)();
};

#ifdef __cplusplus
}
#endif

#endif //CETECH_CONFIG_H
//! \}