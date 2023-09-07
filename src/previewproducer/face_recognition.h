// Copyright (C) 2017  Davis E. King (davis@dlib.net)
// Copyright (C) 2023  Anton Golubev (golubevan@altlinux.org)
// License: Boost Software License

// originally exported only for python

#include <dlib/matrix.h>
#include <dlib/geometry/vector.h>
#include <dlib/dnn.h>
#include <dlib/image_transforms.h>
#include <dlib/image_io.h>
#include <dlib/clustering.h>

using namespace dlib;

class face_recognition_model_v1
{

public:

    face_recognition_model_v1(const std::string& model_filename)
    {
        deserialize(model_filename) >> net;
    }

    matrix<double,0,1> compute_face_descriptor (
        array2d<rgb_pixel> &img,
        const full_object_detection& face,
        const int num_jitters,
        float padding = 0.25
    )
    {
        std::vector<full_object_detection> faces(1, face);
        return compute_face_descriptors(img, faces, num_jitters, padding)[0];
    }

    matrix<double,0,1> compute_face_descriptor_from_aligned_image (
        array2d<rgb_pixel> &img,
        const int num_jitters
    )
    {
        std::vector<array2d<rgb_pixel>*> images{&img};
        return batch_compute_face_descriptors_from_aligned_images(images, num_jitters)[0];
    }

    std::vector<matrix<double,0,1>> compute_face_descriptors (
        array2d<rgb_pixel> &img,
        const std::vector<full_object_detection>& faces,
        const int num_jitters,
        float padding = 0.25
    )
    {
        std::vector<array2d<rgb_pixel>*> batch_img(1, &img);
        std::vector<std::vector<full_object_detection>> batch_faces(1, faces);
        return batch_compute_face_descriptors(batch_img, batch_faces, num_jitters, padding)[0];
    }

    std::vector<std::vector<matrix<double,0,1>>> batch_compute_face_descriptors (
        const std::vector<array2d<rgb_pixel>*>& batch_imgs,
        const std::vector<std::vector<full_object_detection>>& batch_faces,
        const int num_jitters,
        float padding = 0.25
        )
    {
        if (batch_imgs.size() != batch_faces.size())
            throw dlib::error("The array of images and the array of array of locations must be of the same size");

        for (const auto& faces : batch_faces)
        {
            for (const auto& f : faces)
            {
                if (f.num_parts() != 68 && f.num_parts() != 5)
                    throw dlib::error("The full_object_detection must use the iBUG 300W 68 point face landmark style or dlib's 5 point style.");
            }
        }

        dlib::array<matrix<rgb_pixel>> face_chips;
        for (size_t i = 0; i < batch_imgs.size(); ++i)
        {
            auto& faces = batch_faces[i];
            auto& img = batch_imgs[i];

            std::vector<chip_details> dets;
            for (const auto& f : faces)
                dets.push_back(get_face_chip_details(f, 150, padding));
            dlib::array<matrix<rgb_pixel>> this_img_face_chips;
            extract_image_chips(*img, dets, this_img_face_chips);

            for (auto& chip : this_img_face_chips)
                face_chips.push_back(chip);
        }

        std::vector<std::vector<matrix<double,0,1>>> face_descriptors(batch_imgs.size());
        if (num_jitters <= 1)
        {
            // extract descriptors and convert from float vectors to double vectors
            auto descriptors = net(face_chips, 16);
            auto next = std::begin(descriptors);
            for (size_t i = 0; i < batch_faces.size(); ++i)
            {
                for (size_t j = 0; j < batch_faces[i].size(); ++j)
                {
                    face_descriptors[i].push_back(matrix_cast<double>(*next++));
                }
            }
            DLIB_ASSERT(next == std::end(descriptors));
        }
        else
        {
            // extract descriptors and convert from float vectors to double vectors
            auto fimg = std::begin(face_chips);
            for (size_t i = 0; i < batch_faces.size(); ++i)
            {
                for (size_t j = 0; j < batch_faces[i].size(); ++j)
                {
                    auto& r = mean(mat(net(jitter_image(*fimg++, num_jitters), 16)));
                    face_descriptors[i].push_back(matrix_cast<double>(r));
                }
            }
            DLIB_ASSERT(fimg == std::end(face_chips));
        }

        return face_descriptors;
    }

    std::vector<matrix<double,0,1>> batch_compute_face_descriptors_from_aligned_images (
        const std::vector<array2d<rgb_pixel>*>& batch_imgs,
        const int num_jitters
    )
    {
        dlib::array<matrix<rgb_pixel>> face_chips;
        for (auto& img : batch_imgs) {

            matrix<rgb_pixel> image;
            assign_image(image, *img);

            // Check for the size of the image
            if ((image.nr() != 150) || (image.nc() != 150)) {
                throw dlib::error("Unsupported image size, it should be of size 150x150. Also cropping must be done as `dlib.get_face_chip` would do it. \
                That is, centered and scaled essentially the same way.");
            }

            face_chips.push_back(image);
        }

        std::vector<matrix<double,0,1>> face_descriptors;
        if (num_jitters <= 1)
        {
            // extract descriptors and convert from float vectors to double vectors
            auto descriptors = net(face_chips, 16);

            for (auto& des: descriptors) {
                face_descriptors.push_back(matrix_cast<double>(des));
            }
        }
        else
        {
            // extract descriptors and convert from float vectors to double vectors
            for (auto& fimg : face_chips) {
                auto& r = mean(mat(net(jitter_image(fimg, num_jitters), 16)));
                face_descriptors.push_back(matrix_cast<double>(r));
            }
        }
        return face_descriptors;
    }

private:

    dlib::rand rnd;

    std::vector<matrix<rgb_pixel>> jitter_image(
        const matrix<rgb_pixel>& img,
        const int num_jitters
    )
    {
        std::vector<matrix<rgb_pixel>> crops;
        for (int i = 0; i < num_jitters; ++i)
            crops.push_back(dlib::jitter_image(img,rnd));
        return crops;
    }

    template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
    using residual = add_prev1<block<N,BN,1,tag1<SUBNET>>>;

    template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
    using residual_down = add_prev2<avg_pool<2,2,2,2,skip1<tag2<block<N,BN,2,tag1<SUBNET>>>>>>;

    template <int N, template <typename> class BN, int stride, typename SUBNET>
    using block  = BN<con<N,3,3,1,1,relu<BN<con<N,3,3,stride,stride,SUBNET>>>>>;

    template <int N, typename SUBNET> using ares      = relu<residual<block,N,affine,SUBNET>>;
    template <int N, typename SUBNET> using ares_down = relu<residual_down<block,N,affine,SUBNET>>;

    template <typename SUBNET> using alevel0 = ares_down<256,SUBNET>;
    template <typename SUBNET> using alevel1 = ares<256,ares<256,ares_down<256,SUBNET>>>;
    template <typename SUBNET> using alevel2 = ares<128,ares<128,ares_down<128,SUBNET>>>;
    template <typename SUBNET> using alevel3 = ares<64,ares<64,ares<64,ares_down<64,SUBNET>>>>;
    template <typename SUBNET> using alevel4 = ares<32,ares<32,ares<32,SUBNET>>>;

    using anet_type = loss_metric<fc_no_bias<128,avg_pool_everything<
                                alevel0<
                                alevel1<
                                alevel2<
                                alevel3<
                                alevel4<
                                max_pool<3,3,2,2,relu<affine<con<32,7,7,2,2,
                                input_rgb_image_sized<150>
                                >>>>>>>>>>>>;
    anet_type net;
};
