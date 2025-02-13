std::vector<std::string> textures;
int main()
{

    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, "Sponza/Sponza.gltf", &data);

    if (result == cgltf_result_success)
    {
        result = cgltf_load_buffers(&options, data, "Sponza/Sponza.gltf");
        if (result != cgltf_result_success)
        {
            std::cerr << "Failed to load bufffers" << std::endl;
            cgltf_free(data);
            return -1;
        }
		
        // gets the name of textures and stores in list 
        for (size_t i = 0; i < data->images_count; i++)
        {
            cgltf_image* image = &data->images[i];

            char* uri = image->uri;
            if (uri != NULL)
            {
                textures.push_back(image->uri);
            }
        }

        for (auto& img : textures)
        {
            std::cout << "Texture: " << img << std::endl;
        }
}