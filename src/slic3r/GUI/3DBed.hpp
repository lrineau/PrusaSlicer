#ifndef slic3r_3DBed_hpp_
#define slic3r_3DBed_hpp_

#include "GLTexture.hpp"
#include "3DScene.hpp"
#if ENABLE_WORLD_COORDINATE
#include "CoordAxes.hpp"
#else
#include "GLModel.hpp"
#endif // ENABLE_WORLD_COORDINATE
#include "MeshUtils.hpp"

#include "libslic3r/BuildVolume.hpp"
#if ENABLE_LEGACY_OPENGL_REMOVAL
#include "libslic3r/ExPolygon.hpp"
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

#include <tuple>
#include <array>

namespace Slic3r {
namespace GUI {

class GLCanvas3D;

#if !ENABLE_LEGACY_OPENGL_REMOVAL
class GeometryBuffer
{
    struct Vertex
    {
        Vec3f position{ Vec3f::Zero() };
        Vec2f tex_coords{ Vec2f::Zero() };
    };

    std::vector<Vertex> m_vertices;

public:
    bool set_from_triangles(const std::vector<Vec2f> &triangles, float z);
    bool set_from_lines(const Lines& lines, float z);

    const float* get_vertices_data() const;
    unsigned int get_vertices_data_size() const { return (unsigned int)m_vertices.size() * get_vertex_data_size(); }
    unsigned int get_vertex_data_size() const { return (unsigned int)(5 * sizeof(float)); }
    size_t get_position_offset() const { return 0; }
    size_t get_tex_coords_offset() const { return (size_t)(3 * sizeof(float)); }
    unsigned int get_vertices_count() const { return (unsigned int)m_vertices.size(); }
};
#endif // !ENABLE_LEGACY_OPENGL_REMOVAL

class Bed3D
{
#if !ENABLE_WORLD_COORDINATE
    class Axes
    {
    public:
        static const float DefaultStemRadius;
        static const float DefaultStemLength;
        static const float DefaultTipRadius;
        static const float DefaultTipLength;

    private:
        Vec3d m_origin{ Vec3d::Zero() };
        float m_stem_length{ DefaultStemLength };
        GLModel m_arrow;

    public:
        const Vec3d& get_origin() const { return m_origin; }
        void set_origin(const Vec3d& origin) { m_origin = origin; }
        void set_stem_length(float length) {
            m_stem_length = length;
            m_arrow.reset();
        }
        float get_total_length() const { return m_stem_length + DefaultTipLength; }
        void render();
    };
#endif // !ENABLE_WORLD_COORDINATE

public:
    enum class Type : unsigned char
    {
        // The print bed model and texture are available from some printer preset.
        System,
        // The print bed model is unknown, thus it is rendered procedurally.
        Custom
    };

private:
    BuildVolume m_build_volume;
    Type m_type{ Type::Custom };
    std::string m_texture_filename;
    std::string m_model_filename;
    // Print volume bounding box exteded with axes and model.
    BoundingBoxf3 m_extended_bounding_box;
#if ENABLE_LEGACY_OPENGL_REMOVAL
    // Print bed polygon
    ExPolygon m_contour;
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
    // Slightly expanded print bed polygon, for collision detection.
    Polygon m_polygon;
#if ENABLE_LEGACY_OPENGL_REMOVAL
    GLModel m_triangles;
    GLModel m_gridlines;
    GLModel m_contourlines;
#else
    GeometryBuffer m_triangles;
    GeometryBuffer m_gridlines;
    GeometryBuffer m_contourlines;
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
    GLTexture m_texture;
    // temporary texture shown until the main texture has still no levels compressed
    GLTexture m_temp_texture;
    PickingModel m_model;
    Vec3d m_model_offset{ Vec3d::Zero() };
#if !ENABLE_LEGACY_OPENGL_REMOVAL
    unsigned int m_vbo_id{ 0 };
#endif // !ENABLE_LEGACY_OPENGL_REMOVAL
#if ENABLE_WORLD_COORDINATE
    CoordAxes m_axes;
#else
    Axes m_axes;
#endif // ENABLE_WORLD_COORDINATE

    float m_scale_factor{ 1.0f };

public:
    Bed3D() = default;
#if ENABLE_LEGACY_OPENGL_REMOVAL
    ~Bed3D() = default;
#else
    ~Bed3D() { release_VBOs(); }
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    // Update print bed model from configuration.
    // Return true if the bed shape changed, so the calee will update the UI.
    //FIXME if the build volume max print height is updated, this function still returns zero
    // as this class does not use it, thus there is no need to update the UI.
    bool set_shape(const Pointfs& bed_shape, const double max_print_height, const std::string& custom_texture, const std::string& custom_model, bool force_as_custom = false);

    // Build volume geometry for various collision detection tasks.
    const BuildVolume& build_volume() const { return m_build_volume; }

    // Was the model provided, or was it generated procedurally?
    Type get_type() const { return m_type; }
    // Was the model generated procedurally?
    bool is_custom() const { return m_type == Type::Custom; }

    // Bounding box around the print bed, axes and model, for rendering.
    const BoundingBoxf3& extended_bounding_box() const { return m_extended_bounding_box; }

    // Check against an expanded 2d bounding box.
    //FIXME shall one check against the real build volume?
    bool contains(const Point& point) const;
    Point point_projection(const Point& point) const;

#if ENABLE_LEGACY_OPENGL_REMOVAL
    void render(GLCanvas3D& canvas, const Transform3d& view_matrix, const Transform3d& projection_matrix, bool bottom, float scale_factor, bool show_axes, bool show_texture);
    void render_for_picking(GLCanvas3D& canvas, const Transform3d& view_matrix, const Transform3d& projection_matrix, bool bottom, float scale_factor);
#else
    void render(GLCanvas3D& canvas, bool bottom, float scale_factor, bool show_axes, bool show_texture);
    void render_for_picking(GLCanvas3D& canvas, bool bottom, float scale_factor);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

private:
    // Calculate an extended bounding box from axes and current model for visualization purposes.
    BoundingBoxf3 calc_extended_bounding_box() const;
#if ENABLE_LEGACY_OPENGL_REMOVAL
    void init_triangles();
    void init_gridlines();
    void init_contourlines();
#else
    void calc_triangles(const ExPolygon& poly);
    void calc_gridlines(const ExPolygon& poly, const BoundingBox& bed_bbox);
    void calc_contourlines(const ExPolygon& poly);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
    static std::tuple<Type, std::string, std::string> detect_type(const Pointfs& shape);
#if ENABLE_LEGACY_OPENGL_REMOVAL
    void render_internal(GLCanvas3D& canvas, const Transform3d& view_matrix, const Transform3d& projection_matrix, bool bottom, float scale_factor,
        bool show_axes, bool show_texture, bool picking);
#else
    void render_internal(GLCanvas3D& canvas, bool bottom, float scale_factor,
        bool show_axes, bool show_texture, bool picking);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
    void render_axes();
#if ENABLE_LEGACY_OPENGL_REMOVAL
    void render_system(GLCanvas3D& canvas, const Transform3d& view_matrix, const Transform3d& projection_matrix, bool bottom, bool show_texture);
    void render_texture(bool bottom, GLCanvas3D& canvas, const Transform3d& view_matrix, const Transform3d& projection_matrix);
    void render_model(const Transform3d& view_matrix, const Transform3d& projection_matrix);
    void render_custom(GLCanvas3D& canvas, const Transform3d& view_matrix, const Transform3d& projection_matrix, bool bottom, bool show_texture, bool picking);
    void render_default(bool bottom, bool picking, bool show_texture, const Transform3d& view_matrix, const Transform3d& projection_matrix);
    void render_contour(const Transform3d& view_matrix, const Transform3d& projection_matrix);
#else
    void render_system(GLCanvas3D& canvas, bool bottom, bool show_texture);
    void render_texture(bool bottom, GLCanvas3D& canvas);
    void render_model();
    void render_custom(GLCanvas3D& canvas, bool bottom, bool show_texture, bool picking);
    void render_default(bool bottom, bool picking, bool show_texture);
    void render_contour();

    void release_VBOs();
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    void register_raycasters_for_picking(const GLModel::Geometry& geometry, const Transform3d& trafo);
};

} // GUI
} // Slic3r

#endif // slic3r_3DBed_hpp_
